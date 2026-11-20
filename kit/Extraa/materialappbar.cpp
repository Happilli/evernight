#include "./materialappbar.hpp"
#include <QFontMetricsF>
#include <QQmlEngine>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kRow = 64.0;
constexpr qreal kTarget = 48.0;
constexpr qreal kEdge = 4.0;
constexpr qreal kIcon = 24.0;
constexpr qreal kLayerR = 20.0;
constexpr qreal kPad = 16.0;
constexpr qreal kTitleAfterNav = 72.0;
constexpr qreal kCollapsedPx = 22.0;
constexpr qreal kSubSmallPx = 12.0;
constexpr qreal kSubPx = 14.0;
constexpr qreal kBottomPad = 20.0;
constexpr qreal kSubLh = 20.0;
constexpr qreal kLineK = 1.3;
constexpr qreal kStroke = 2.0;
constexpr qreal kStiffness = 1.0;
constexpr qreal kMenuHalf = 9.0;
constexpr qreal kMenuGap = 6.0;
constexpr qreal kDotR = 2.0;
constexpr int kSlotTitle = 0;
constexpr int kSlotSub = 1;
constexpr int kSlotNav = 2;
constexpr int kSlotAction = 3;

qreal lerp(qreal a, qreal b, qreal t) { return a + (b - a) * t; }
} // namespace

MaterialAppBar::MaterialAppBar(QQuickItem *parent)
    : SgMaterialItem(parent, 520.0, 0.6) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setImplicitWidth(360.0);
  updateImplicit();
  retarget(true);
}

void MaterialAppBar::componentComplete() {
  SgMaterialItem::componentComplete();
  updateImplicit();
  retarget(true);
  update();
}

void MaterialAppBar::geometryChange(const QRectF &n, const QRectF &o) {
  SgMaterialItem::geometryChange(n, o);
  if (n.size() != o.size())
    update();
}

qreal MaterialAppBar::expandedHeight() const {
  switch (m_type) {
  case Small:
    return 64.0;
  case Medium:
    return 112.0;
  case Large:
    return 152.0;
  }
  return 64.0;
}

qreal MaterialAppBar::collapsedHeight() const { return kRow; }

qreal MaterialAppBar::collapseRange() const { return expandedHeight() - kRow; }

qreal MaterialAppBar::expandedPx() const {
  switch (m_type) {
  case Small:
    return kCollapsedPx;
  case Medium:
    return 24.0;
  case Large:
    return 28.0;
  }
  return kCollapsedPx;
}

qreal MaterialAppBar::currentHeight() const {
  const qreal t = flexible() ? std::clamp(m_collapse, 0.0, 1.0) : 0.0;
  return lerp(expandedHeight(), kRow, t);
}

bool MaterialAppBar::hasNavigation() const {
  return m_nav != NoNavigation || !m_navSource.isEmpty();
}

qreal MaterialAppBar::actionCx(int i) const {
  const int n = int(m_actions.size());
  return width() - kEdge - kTarget / 2.0 - (n - 1 - i) * kTarget;
}

int MaterialAppBar::hit(const QPointF &pt) const {
  if (pt.y() < 0.0 || pt.y() > kRow)
    return None;
  if (hasNavigation() && pt.x() >= kEdge && pt.x() <= kEdge + kTarget)
    return Nav;
  for (int i = 0; i < int(m_actions.size()); ++i)
    if (m_actions[i].enabled && std::abs(pt.x() - actionCx(i)) <= kTarget / 2.0)
      return 1 + i;
  return None;
}

void MaterialAppBar::updateImplicit() {
  setImplicitHeight(std::round(currentHeight()));
}

void MaterialAppBar::retarget(bool immediate) {
  m_scr.go(m_scrolled ? 1.0 : 0.0, immediate);
  if (!immediate)
    animate();
}

bool MaterialAppBar::advance(qreal dt) {
  return step(m_scr, dt, 0.001, kStiffness);
}

void MaterialAppBar::setTitle(const QString &v) {
  assign(m_title, v, &MaterialAppBar::titleChanged);
}

void MaterialAppBar::setSubtitle(const QString &v) {
  assign(m_subtitle, v, &MaterialAppBar::subtitleChanged);
}

void MaterialAppBar::setType(Type v) {
  if (assign(m_type, v, &MaterialAppBar::typeChanged))
    updateImplicit();
}

void MaterialAppBar::setNavigation(Navigation v) {
  assign(m_nav, v, &MaterialAppBar::navigationChanged);
}

void MaterialAppBar::setNavigationIconSource(const QUrl &v) {
  if (assign(m_navSource, v, &MaterialAppBar::navigationIconSourceChanged)) {
    m_navPath.clear();
    m_navCache.invalidate();
  }
}

void MaterialAppBar::setActions(const QVariantList &v) {
  m_raw = v;
  m_actions.clear();
  m_actions.reserve(v.size());
  for (const QVariant &it : v) {
    Action a;
    if (it.metaType().id() == QMetaType::QString) {
      const QString s = it.toString();
      if (s == QLatin1String("more"))
        a.kind = s;
      else
        a.glyph = s;
    } else {
      const QVariantMap mp = it.toMap();
      a.kind = mp.value("kind").toString();
      a.glyph = mp.value("glyph").toString();
      const QString icon = mp.value("icon").toString();
      if (!icon.isEmpty())
        a.src = QUrl(icon);
      a.enabled = mp.contains("enabled") ? mp.value("enabled").toBool() : true;
    }
    m_actions.push_back(std::move(a));
  }
  m_pressZone = None;
  emit actionsChanged();
  update();
}

void MaterialAppBar::setCenterTitle(bool v) {
  assign(m_centerTitle, v, &MaterialAppBar::centerTitleChanged);
}

void MaterialAppBar::setCollapse(qreal v) {
  v = std::clamp(v, 0.0, 1.0);
  if (assign(m_collapse, v, &MaterialAppBar::collapseChanged))
    updateImplicit();
}

void MaterialAppBar::setScrolled(bool v) {
  if (assign(m_scrolled, v, &MaterialAppBar::scrolledChanged))
    retarget();
}

void MaterialAppBar::setFont(const QFont &v) {
  assign(m_font, v, &MaterialAppBar::fontChanged);
}

void MaterialAppBar::setIconFont(const QFont &v) {
  assign(m_iconFont, v, &MaterialAppBar::iconFontChanged);
}

void MaterialAppBar::setFontWeight(int v) {
  v = qBound(1, v, 1000);
  assign(m_fontWeight, v, &MaterialAppBar::fontWeightChanged);
}

void MaterialAppBar::setColor(const QColor &v) {
  assign(m_color, v, &MaterialAppBar::colorChanged);
}

void MaterialAppBar::setScrolledColor(const QColor &v) {
  assign(m_scrolledColor, v, &MaterialAppBar::scrolledColorChanged);
}

void MaterialAppBar::setTitleColor(const QColor &v) {
  assign(m_titleColor, v, &MaterialAppBar::titleColorChanged);
}

void MaterialAppBar::setSubtitleColor(const QColor &v) {
  assign(m_subtitleColor, v, &MaterialAppBar::subtitleColorChanged);
}

void MaterialAppBar::setIconColor(const QColor &v) {
  assign(m_iconColor, v, &MaterialAppBar::iconColorChanged);
}

void MaterialAppBar::setPressZone(int z) {
  if (m_pressZone == z)
    return;
  m_pressZone = z;
  update();
}

void MaterialAppBar::activate(int zone) {
  if (zone == Nav) {
    emit navigationClicked();
    return;
  }
  const int i = zone - 1;
  if (i < 0 || i >= int(m_actions.size()))
    return;
  emit actionClicked(i, m_raw.value(i),
                     QPointF(actionCx(i) + kTarget / 2.0, kRow - 8.0));
}

void MaterialAppBar::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled()) {
    e->ignore();
    return;
  }
  const int z = hit(e->position());
  if (z == None) {
    e->ignore();
    return;
  }
  setPressZone(z);
  e->accept();
}

void MaterialAppBar::mouseReleaseEvent(QMouseEvent *e) {
  if (m_pressZone == None)
    return;
  const int pressed = m_pressZone;
  const bool inside = hit(e->position()) == pressed;
  setPressZone(None);
  if (inside)
    activate(pressed);
  e->accept();
}

void MaterialAppBar::mouseUngrabEvent() { setPressZone(None); }

void MaterialAppBar::drawNavigation(Kit::Mesh &m, const QPointF &c,
                                    const QColor &col) {
  const qreal lw = m.snapStroke(kStroke);
  switch (m_nav) {
  case Back:
    m.line(QPointF(c.x() - 8.0, c.y()), QPointF(c.x() + 8.0, c.y()), lw, col);
    m.polyline({QPointF(c.x(), c.y() - 8.0), QPointF(c.x() - 8.0, c.y()),
                QPointF(c.x(), c.y() + 8.0)},
               lw, col);
    break;
  case Menu:
    for (int k = -1; k <= 1; ++k)
      m.line(QPointF(c.x() - kMenuHalf, c.y() + k * kMenuGap),
             QPointF(c.x() + kMenuHalf, c.y() + k * kMenuGap), lw, col);
    break;
  case Close:
    m.line(QPointF(c.x() - 7.0, c.y() - 7.0), QPointF(c.x() + 7.0, c.y() + 7.0),
           lw, col);
    m.line(QPointF(c.x() - 7.0, c.y() + 7.0), QPointF(c.x() + 7.0, c.y() - 7.0),
           lw, col);
    break;
  case NoNavigation:
    break;
  }
}

void MaterialAppBar::imageLayer(Kit::Mesh &m, int slot, Kit::IconCache &cache,
                                QString &path, const QUrl &src,
                                const QColor &col, const QPointF &c) {
  if (path.isEmpty())
    path = Kit::resolveUrl(qmlContext(this), src);
  const qreal dpr = m.dpr();
  const QColor cc = Kit::withAlpha(col, col.alphaF() * fade());
  const QString key = QStringLiteral("i|%1|%2|%3|%4")
                          .arg(path, cc.name(QColor::HexArgb))
                          .arg(kIcon)
                          .arg(dpr);
  useLayer(slot, key,
           QRectF(m.snap(c.x() - kIcon / 2.0), m.snap(c.y() - kIcon / 2.0),
                  kIcon, kIcon),
           [&] { return cache.get(path, kIcon, dpr, cc); });
}

void MaterialAppBar::build(Kit::Mesh &m) {
  const qreal w = width();
  const qreal h = height();
  if (w <= 0.0 || h <= 0.0)
    return;

  const bool small = m_type == Small;
  const qreal t = small ? 0.0 : std::clamp(m_collapse, 0.0, 1.0);
  const qreal scr = std::clamp(std::max(t, m_scr.x), 0.0, 1.0);
  m.fillRoundedRect(QRectF(0.0, 0.0, w, h), 0.0, 0.0, 0.0, 0.0,
                    Kit::mix(m_color, m_scrolledColor, scr));

  const qreal cy = kRow / 2.0;
  const int n = int(m_actions.size());

  if (hasNavigation()) {
    const QPointF c(kEdge + kTarget / 2.0, cy);
    const qreal la = Kit::stateLayer(m_pressZone == Nav);
    if (la > 0.0)
      m.fillCircle(c, kLayerR, Kit::withAlpha(m_iconColor, la));
    if (!m_navSource.isEmpty())
      imageLayer(m, kSlotNav, m_navCache, m_navPath, m_navSource, m_iconColor,
                 c);
    else
      drawNavigation(m, c, m_iconColor);
  }

  for (int i = 0; i < n; ++i) {
    Action &a = m_actions[i];
    const QPointF c(actionCx(i), cy);
    const QColor col =
        a.enabled ? m_iconColor
                  : Kit::withAlpha(m_iconColor, m_iconColor.alphaF() *
                                                    Kit::kDisabledOpacity);
    const qreal la = Kit::stateLayer(m_pressZone == 1 + i);
    if (la > 0.0)
      m.fillCircle(c, kLayerR, Kit::withAlpha(col, la));
    if (a.kind == QLatin1String("more")) {
      for (int k = -1; k <= 1; ++k)
        m.fillCircle(QPointF(c.x(), c.y() + k * kMenuGap), kDotR, col);
    } else if (!a.src.isEmpty()) {
      imageLayer(m, kSlotAction + i, a.cache, a.path, a.src, col, c);
    } else if (!a.glyph.isEmpty()) {
      glyphLayer(kSlotAction + i, m_iconFont, a.glyph, col, c, kIcon);
    }
  }

  const bool hasSub = !m_subtitle.isEmpty();
  const qreal expH = expandedHeight();
  const qreal expPx = expandedPx();
  const qreal px = lerp(expPx, kCollapsedPx, t);
  const qreal subPx = small ? kSubSmallPx : kSubPx;

  qreal titleCy;
  qreal subCy;
  if (small) {
    titleCy = hasSub ? 24.0 : 32.0;
    subCy = 46.0;
  } else {
    const qreal subExp = expH - kBottomPad - kSubLh / 2.0;
    const qreal titleExp = hasSub ? subExp - kSubLh / 2.0 - expPx * kLineK / 2.0
                                  : expH - kBottomPad - expPx * kLineK / 2.0;
    titleCy = lerp(titleExp, 32.0, t);
    subCy = lerp(subExp, 40.0, t);
  }

  const qreal navX = hasNavigation() ? kTitleAfterNav : kPad;
  const qreal collapsedRight =
      n > 0 ? w - (n * kTarget + kEdge) - 8.0 : w - kPad;
  qreal tx = small ? navX : lerp(kPad, navX, t);
  const qreal right =
      small ? collapsedRight : lerp(w - kPad, collapsedRight, t);
  const qreal avail = std::max(0.0, right - tx);

  const QFont tf = Kit::styledFont(m_font, px, m_fontWeight);
  const QFontMetricsF tm(tf);
  const QString title = tm.elidedText(m_title, Qt::ElideRight, avail);
  if (m_centerTitle && small) {
    const qreal tw = tm.horizontalAdvance(title);
    tx = std::clamp((w - tw) / 2.0, tx, std::max(tx, right - tw));
  }
  textLayer(kSlotTitle, tf, title, m_titleColor, tx, titleCy);

  const qreal sa =
      small ? 1.0 : std::round(std::clamp(1.0 - t * 2.5, 0.0, 1.0) * 8.0) / 8.0;
  if (hasSub && sa > 0.0) {
    const QFont sf = Kit::styledFont(m_font, subPx, QFont::Normal);
    const QString sub =
        QFontMetricsF(sf).elidedText(m_subtitle, Qt::ElideRight, avail);
    textLayer(kSlotSub, sf, sub,
              Kit::withAlpha(m_subtitleColor, m_subtitleColor.alphaF() * sa),
              tx, subCy);
  }
}
