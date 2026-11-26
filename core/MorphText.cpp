#include "./MorphText.h"
#include <QEasingCurve>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGOpacityNode>
#include <QSGTexture>
#include <QSGTextureMaterial>
#include <algorithm>
#include <cmath>
#include <memory>

namespace {
constexpr int kPad = 2;
constexpr qreal kRise = 6.0;
} // namespace

class MorphNode : public QSGNode {
public:
  MorphNode() {
    for (int k = 0; k < 3; ++k) {
      op[k] = new QSGOpacityNode;
      auto *g =
          new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0);
      g->setDrawingMode(QSGGeometry::DrawTriangles);
      g->setVertexDataPattern(QSGGeometry::DynamicPattern);
      mat[k] = new QSGTextureMaterial;
      mat[k]->setFiltering(QSGTexture::Linear);
      geo[k] = new QSGGeometryNode;
      geo[k]->setGeometry(g);
      geo[k]->setFlag(QSGNode::OwnsGeometry);
      geo[k]->setMaterial(mat[k]);
      geo[k]->setFlag(QSGNode::OwnsMaterial);
      op[k]->appendChildNode(geo[k]);
      appendChildNode(op[k]);
    }
  }
  bool hasTexture() const { return bool(tex); }
  void setTexture(QSGTexture *t) {
    for (int k = 0; k < 3; ++k) {
      mat[k]->setTexture(t);
      geo[k]->markDirty(QSGNode::DirtyMaterial);
    }
    tex.reset(t);
  }
  QSGOpacityNode *op[3];
  QSGGeometryNode *geo[3];
  QSGTextureMaterial *mat[3];
  std::unique_ptr<QSGTexture> tex;
};

MorphText::MorphText(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
  updateImplicit();
}

void MorphText::updateImplicit() {
  setImplicitHeight(std::ceil(QFontMetricsF(m_font).height()));
}

void MorphText::setText(const QString &v) {
  if (m_text == v)
    return;
  m_text = v;
  emit textChanged();
  rebuild(true);
}

void MorphText::setColor(const QColor &v) {
  if (m_color == v)
    return;
  m_color = v;
  m_atlasDirty = true;
  emit colorChanged();
  update();
}

void MorphText::setFont(const QFont &v) {
  if (m_font == v)
    return;
  m_font = v;
  updateImplicit();
  emit fontChanged();
  rebuild(false);
}

void MorphText::setDuration(int v) {
  if (m_duration == v)
    return;
  m_duration = v;
  emit durationChanged();
}

void MorphText::setProgress(qreal v) {
  if (m_progress == v)
    return;
  m_progress = v;
  emit progressChanged();
  update();
}

void MorphText::geometryChange(const QRectF &n, const QRectF &o) {
  QQuickItem::geometryChange(n, o);
  if (n.width() != o.width())
    rebuild(false);
}

std::vector<MorphText::G> MorphText::layout(const QString &s) const {
  QFontMetricsF fm(m_font);
  std::vector<G> out;
  out.reserve(size_t(s.size()));
  for (int i = 0; i < s.size(); ++i)
    out.push_back({s[i], fm.horizontalAdvance(s.left(i))});
  return out;
}

void MorphText::rebuild(bool animate) {
  const QString str =
      width() > 0
          ? QFontMetricsF(m_font).elidedText(m_text, Qt::ElideRight, width())
          : m_text;
  QString shown;
  for (const Item &i : m_items)
    if (i.kind != 2)
      shown += i.ch;
  const bool anim =
      animate && !m_items.empty() && width() > 0 && isVisible() && window();
  if (anim && shown == str)
    return;
  const std::vector<G> to = layout(str);
  std::vector<Item> next;
  if (!anim) {
    next.reserve(to.size());
    for (const G &g : to)
      next.push_back({g.ch, g.x, g.x, 0});
  } else {
    std::vector<G> from;
    for (const Item &i : m_items)
      if (i.kind != 2)
        from.push_back({i.ch, i.fromX + (i.toX - i.fromX) * m_progress});
    const int n = int(from.size());
    const int m = int(to.size());
    std::vector<int> dp(size_t(n + 1) * size_t(m + 1), 0);
    auto at = [&](int a, int b) -> int & {
      return dp[size_t(a) * size_t(m + 1) + size_t(b)];
    };
    for (int a = n - 1; a >= 0; --a)
      for (int b = m - 1; b >= 0; --b)
        at(a, b) = from[a].ch == to[b].ch
                       ? at(a + 1, b + 1) + 1
                       : std::max(at(a + 1, b), at(a, b + 1));
    int i = 0, j = 0;
    while (i < n && j < m) {
      if (from[i].ch == to[j].ch) {
        next.push_back({to[j].ch, from[i].x, to[j].x, 0});
        ++i;
        ++j;
      } else if (at(i + 1, j) >= at(i, j + 1)) {
        next.push_back({from[i].ch, from[i].x, from[i].x, 2});
        ++i;
      } else {
        next.push_back({to[j].ch, to[j].x, to[j].x, 1});
        ++j;
      }
    }
    for (; i < n; ++i)
      next.push_back({from[i].ch, from[i].x, from[i].x, 2});
    for (; j < m; ++j)
      next.push_back({to[j].ch, to[j].x, to[j].x, 1});
  }
  m_run = false;
  m_items = std::move(next);
  m_atlasDirty = true;
  if (anim) {
    m_progress = 0.0;
    m_vel = 0.0;
    m_run = true;
    m_clock.restart();
    emit progressChanged();
  } else {
    m_progress = 1.0;
    m_vel = 0.0;
    emit progressChanged();
  }
  update();
}

void MorphText::buildAtlas(MorphNode *n) {
  m_dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
  QFontMetricsF fm(m_font);
  m_cellPx.clear();
  int x = 0;
  const int padPx = kPad;
  const int cellH = int(std::ceil((fm.height() + 2 * kPad) * m_dpr));
  std::vector<std::pair<ushort, int>> cells;
  for (const Item &it : m_items) {
    const ushort u = it.ch.unicode();
    if (m_cellPx.contains(u))
      continue;
    const int w =
        int(std::ceil(fm.horizontalAdvance(it.ch) * m_dpr)) + 2 * padPx;
    m_cellPx.insert(u, x);
    cells.push_back({u, w});
    x += w;
  }
  m_atlasW = std::max(1, x);
  m_atlasH = std::max(1, cellH);
  m_cellH = m_atlasH;
  QImage img(m_atlasW, m_atlasH, QImage::Format_ARGB32_Premultiplied);
  img.fill(Qt::transparent);
  {
    QPainter p(&img);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(m_font);
    p.setPen(m_color);
    p.scale(m_dpr, m_dpr);
    for (const auto &c : cells) {
      const qreal px = (m_cellPx.value(c.first) + padPx) / m_dpr;
      p.drawText(QPointF(px, kPad + fm.ascent()), QString(QChar(c.first)));
    }
  }
  QSGTexture *t = window()->createTextureFromImage(img);
  n->setTexture(t);
  m_atlasDirty = false;
}

QSGNode *MorphText::updatePaintNode(QSGNode *old, UpdatePaintNodeData *) {
  auto *n = static_cast<MorphNode *>(old);
  if (!n)
    n = new MorphNode;
  if (!window())
    return n;
  QFontMetricsF fm(m_font);
  if (m_atlasDirty || !n->hasTexture())
    buildAtlas(n);
  if (m_run) {
    qreal dt = std::min(0.05, m_clock.restart() / 1000.0);
    const int steps = std::max(1, int(std::ceil(dt / 0.004)));
    const qreal h = dt / steps;
    for (int s = 0; s < steps; ++s) {
      const qreal a = -m_k * (m_progress - 1.0) - m_c * m_vel;
      m_vel += a * h;
      m_progress += m_vel * h;
    }
    if (std::abs(m_progress - 1.0) < 0.0005 && std::abs(m_vel) < 0.005) {
      m_progress = 1.0;
      m_vel = 0.0;
      m_run = false;
    } else {
      QMetaObject::invokeMethod(
          this, [this] { update(); }, Qt::QueuedConnection);
    }
  }
  const qreal t = m_progress;
  const qreal tc = std::clamp(t, 0.0, 1.0);
  const qreal opIn = std::clamp((tc - 0.3) / 0.7, 0.0, 1.0);
  const qreal opOut = std::clamp(1.0 - tc * 2.2, 0.0, 1.0);
  n->op[0]->setOpacity(1.0);
  n->op[1]->setOpacity(opIn);
  n->op[2]->setOpacity(opOut);
  const qreal cellHl = m_cellH / m_dpr;
  for (int k = 0; k < 3; ++k) {
    int cnt = 0;
    for (const Item &it : m_items)
      if (it.kind == k && it.ch != QLatin1Char(' '))
        ++cnt;
    QSGGeometry *g = n->geo[k]->geometry();
    g->allocate(cnt * 6);
    auto *v = g->vertexDataAsTexturedPoint2D();
    int q = 0;
    for (const Item &it : m_items) {
      if (it.kind != k || it.ch == QLatin1Char(' '))
        continue;
      const int cx = m_cellPx.value(it.ch.unicode());
      const int cw =
          int(std::ceil(fm.horizontalAdvance(it.ch) * m_dpr)) + 2 * kPad;
      const qreal gx = it.fromX + (it.toX - it.fromX) * t;
      const qreal yo = k == 1 ? (1.0 - t) * kRise : (k == 2 ? -t * kRise : 0.0);
      const qreal x0 = gx - kPad / m_dpr;
      const qreal x1 = x0 + cw / m_dpr;
      const qreal y0 = yo - kPad;
      const qreal y1 = y0 + cellHl;
      const float u0 = float(cx) / m_atlasW;
      const float u1 = float(cx + cw) / m_atlasW;
      QSGGeometry::TexturedPoint2D *p = v + q * 6;
      p[0].set(float(x0), float(y0), u0, 0.f);
      p[1].set(float(x1), float(y0), u1, 0.f);
      p[2].set(float(x0), float(y1), u0, 1.f);
      p[3].set(float(x1), float(y0), u1, 0.f);
      p[4].set(float(x1), float(y1), u1, 1.f);
      p[5].set(float(x0), float(y1), u0, 1.f);
      ++q;
    }
    n->geo[k]->markDirty(QSGNode::DirtyGeometry);
  }
  return n;
}
