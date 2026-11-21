#include "./materiallist.hpp"
#include "../Core/paintutil.hpp"
#include <QCache>
#include <QCoreApplication>
#include <QFontMetricsF>
#include <QImageReader>
#include <QMetaObject>
#include <QMutexLocker>
#include <QPainter>
#include <QPainterPath>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kHeight[] = {56.0, 56.0, 72.0, 88.0};
constexpr qreal kPad = 16.0;
constexpr qreal kGap = 16.0;
constexpr qreal kIcon = 24.0;
constexpr qreal kAvatar = 40.0;
constexpr qreal kImage = 56.0;
constexpr qreal kImageR = 8.0;
constexpr qreal kOuterR = 16.0;
constexpr qreal kInnerR = 4.0;
constexpr qreal kSegGap = 2.0;
constexpr qreal kSegDivider = 8.0;
constexpr qreal kDivider = 1.0;
constexpr qreal kOverlineLh = 16.0;
constexpr qreal kHeadlineLh = 24.0;
constexpr qreal kSupportLh = 20.0;
constexpr qreal kEqBarW = 4.0;
constexpr qreal kEqBarGap = 3.0;
constexpr qreal kEqMinH = 4.0;
constexpr qreal kBuffer = 160.0;
constexpr int kWindow = 64;
constexpr int kSlots = 8;
constexpr int kSlotImage = 0;
constexpr int kSlotAvatar = 1;
constexpr int kSlotLeading = 2;
constexpr int kSlotTrailingIcon = 3;
constexpr int kSlotTrailingText = 4;
constexpr int kSlotOverline = 5;
constexpr int kSlotHeadline = 6;
constexpr int kSlotSupporting = 7;

QCache<QString, QImage> &thumbCache() {
  static QCache<QString, QImage> cache(24 * 1024);
  return cache;
}

QThreadPool &thumbPool() {
  static QThreadPool pool;
  static const bool init = (pool.setMaxThreadCount(2), true);
  Q_UNUSED(init)
  return pool;
}

QImage loadThumb(const QString &path, int px, qreal dpr) {
  QImageReader r(path);
  const QSize s = r.size();
  if (s.isValid() && (s.width() > px || s.height() > px)) {
    QSize t = s;
    t.scale(px, px, Qt::KeepAspectRatioByExpanding);
    r.setScaledSize(t);
  }
  const QImage in = r.read();
  if (in.isNull())
    return {};
  const int side = std::min(in.width(), in.height());
  const QRect src((in.width() - side) / 2, (in.height() - side) / 2, side,
                  side);
  QImage out(px, px, QImage::Format_ARGB32_Premultiplied);
  out.fill(Qt::transparent);
  QPainter p(&out);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  QPainterPath clip;
  clip.addRoundedRect(QRectF(0.0, 0.0, px, px), kImageR * dpr, kImageR * dpr);
  p.setClipPath(clip);
  p.drawImage(QRectF(0.0, 0.0, px, px), in, src);
  p.end();
  out.setDevicePixelRatio(dpr);
  return out;
}
} // namespace

MaterialList::MaterialList(QQuickItem *parent)
    : SgMaterialItem(parent, 520.0, 0.6) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setImplicitWidth(360.0);
  structureChanged();
  connect(this, &MaterialList::checkedIndicesChanged, this,
          [this] { syncHighlight(); });
  m_holdTimer.setSingleShot(true);
  m_holdTimer.setInterval(450);
  connect(&m_holdTimer, &QTimer::timeout, this, [this] {
    if (m_pressIndex < 0)
      return;
    const int i = m_pressIndex;
    m_pressIndex = -1;
    m_held = true;
    m_press.go(0.0);
    animate();
    update();
    emit longPressed(i, m_raw.value(i));
  });
}

void MaterialList::componentComplete() {
  SgMaterialItem::componentComplete();
  attachFlickable();
  update();
}

void MaterialList::attachFlickable() {
  QQuickItem *found = nullptr;
  for (QQuickItem *p = parentItem(); p; p = p->parentItem()) {
    const QMetaObject *mo = p->metaObject();
    if (mo->indexOfProperty("contentY") >= 0 &&
        mo->indexOfSignal("contentYChanged()") >= 0) {
      found = p;
      break;
    }
  }
  if (found == m_flick.data())
    return;
  if (m_flickConn)
    disconnect(m_flickConn);
  m_flickConn = {};
  m_flick = found;
  if (found)
    m_flickConn =
        connect(found, SIGNAL(contentYChanged()), this, SLOT(update()));
}

void MaterialList::itemChange(ItemChange change, const ItemChangeData &data) {
  SgMaterialItem::itemChange(change, data);
  if (change == ItemParentHasChanged)
    attachFlickable();
}

void MaterialList::geometryChange(const QRectF &n, const QRectF &o) {
  QQuickItem::geometryChange(n, o);
  if (n.size() != o.size())
    update();
}

QColor MaterialList::resolvedItemColor() const {
  if (m_itemColor.isValid())
    return m_itemColor;
  return m_variant == Segmented ? QColor("#F3EDF7") : QColor(Qt::transparent);
}

qreal MaterialList::itemGap() const {
  return m_variant == Segmented ? kSegGap : 0.0;
}

qreal MaterialList::rowHeight(int i) const {
  const Entry &e = m_items[i];
  if (e.divider)
    return m_variant == Segmented ? kSegDivider : kDivider;
  return kHeight[std::min(e.lines(), 3)];
}

void MaterialList::relayout() {
  const int n = int(m_items.size());
  m_tops.assign(n, 0.0);
  m_hts.assign(n, 0.0);
  qreal y = 0.0;
  for (int i = 0; i < n; ++i) {
    const qreal h = rowHeight(i);
    m_tops[i] = y;
    m_hts[i] = h;
    y += h + (m_items[i].divider ? 0.0 : itemGap());
  }
  if (n > 0 && !m_items.back().divider)
    y -= itemGap();
  setImplicitHeight(std::ceil(std::max(0.0, y)));
  update();
  syncHighlight();
}

void MaterialList::structureChanged() {
  m_checked.erase(std::remove_if(m_checked.begin(), m_checked.end(),
                                 [this](int i) { return !selectable(i); }),
                  m_checked.end());
  relayout();
}

void MaterialList::syncHighlight() {
  const int i = m_checked.isEmpty() ? -1 : m_checked.first();
  if (m_mode != Single || i < 0 || i >= int(m_tops.size()) ||
      m_items[i].divider) {
    m_hlA.go(0.0);
    animate();
    return;
  }
  const bool snap = m_hlA.x < 0.02;
  m_hlY.go(m_tops[i], snap);
  m_hlH.go(m_hts[i], snap);
  m_hlA.go(1.0);
  animate();
}

bool MaterialList::selectable(int i) const {
  return i >= 0 && i < int(m_items.size()) && !m_items[i].divider &&
         m_items[i].enabled;
}

int MaterialList::rowAt(const QPointF &pt) const {
  if (m_tops.empty() || pt.y() < 0.0)
    return -1;
  const auto it = std::upper_bound(m_tops.begin(), m_tops.end(), pt.y());
  if (it == m_tops.begin())
    return -1;
  const int i = int(it - m_tops.begin()) - 1;
  const qreal bottom =
      m_tops[i] + m_hts[i] + (m_items[i].divider ? 0.0 : itemGap());
  return pt.y() < bottom ? i : -1;
}

bool MaterialList::visibleRows(int &first, int &last) const {
  const int n = int(m_items.size());
  if (n == 0)
    return false;
  if (n <= kWindow) {
    first = 0;
    last = n - 1;
    return true;
  }
  qreal top = 0.0;
  qreal bottom = height();
  if (window()) {
    const QRectF r = mapRectFromScene(
        QRectF(0.0, 0.0, window()->width(), window()->height()));
    top = std::max(0.0, r.top() - kBuffer);
    bottom = std::min(height(), r.bottom() + kBuffer);
  }
  if (bottom <= top)
    return false;
  const auto lo = std::upper_bound(m_tops.begin(), m_tops.end(), top);
  first = std::max(0, int(lo - m_tops.begin()) - 1);
  const auto hi = std::lower_bound(m_tops.begin(), m_tops.end(), bottom);
  last = std::min(n - 1, int(hi - m_tops.begin()) - 1);
  return last >= first;
}

void MaterialList::activate(int i) {
  if (!selectable(i))
    return;
  const QVariant item = m_raw.value(i);
  if (m_mode == Single) {
    if (!isChecked(i)) {
      m_checked = {i};
      emit checkedIndicesChanged();
    }
  } else if (m_mode == Multiple) {
    if (isChecked(i))
      m_checked.removeAll(i);
    else {
      m_checked.append(i);
      std::sort(m_checked.begin(), m_checked.end());
    }
    emit checkedIndicesChanged();
  }
  update();
  QMetaObject::invokeMethod(
      this, [this, i, item] { emit clicked(i, item); }, Qt::QueuedConnection);
}

void MaterialList::requestThumb(const QString &key, const QString &path, int px,
                                qreal dpr) {
  if (m_pending.contains(key))
    return;
  m_pending.insert(key);
  {
    QMutexLocker lock(&m_wanted->m);
    m_wanted->s.insert(key);
  }
  static int serial = 0;
  QPointer<MaterialList> self(this);
  std::shared_ptr<Wanted> wanted = m_wanted;
  thumbPool().start(
      [self, key, path, px, dpr, wanted] {
        bool want;
        {
          QMutexLocker lock(&wanted->m);
          want = wanted->s.contains(key);
        }
        QImage img;
        if (want)
          img = loadThumb(path, px, dpr);
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [self, key, img, want] {
              if (!self)
                return;
              self->m_pending.remove(key);
              if (want)
                thumbCache().insert(
                    key, new QImage(img),
                    img.isNull() ? 1 : int(img.sizeInBytes() / 1024) + 1);
              self->update();
            },
            Qt::QueuedConnection);
      },
      ++serial);
}

bool MaterialList::advance(qreal dt) {
  bool active = step(m_press, dt);
  active |= Kit::stepSpring(m_hlY, springStiffness(), 1.0, dt, 0.05);
  active |= Kit::stepSpring(m_hlH, springStiffness(), 1.0, dt, 0.05);
  active |= Kit::stepSpring(m_hlA, springStiffness(), 1.0, dt, 0.001);
  if (!active && m_press.t == 0.0)
    m_pressShown = -1;
  return active;
}

void MaterialList::setModel(const QVariantList &v) {
  m_raw = v;
  m_items.clear();
  m_items.reserve(size_t(v.size()));
  for (const QVariant &it : v) {
    Entry e;
    if (it.metaType().id() == QMetaType::QString) {
      const QString s = it.toString();
      if (s == QLatin1String("-") || s == QLatin1String("---"))
        e.divider = true;
      else
        e.headline = s;
    } else {
      const QVariantMap m = it.toMap();
      e.divider = m.value("divider").toBool();
      e.overline = m.value("overline").toString();
      e.headline = m.value("headline").toString();
      e.supporting = m.value("supporting").toString();
      e.trailingText = m.value("trailingText").toString();
      e.leadingIcon = m.value("leadingIcon").toString();
      e.trailingIcon = m.value("trailingIcon").toString();
      e.avatarText = m.value("avatarText").toString();
      e.enabled = m.contains("enabled") ? m.value("enabled").toBool() : true;
      const QString img = m.value("image").toString();
      if (!img.isEmpty())
        e.imagePath = img.startsWith(QLatin1Char('/'))
                          ? img
                          : Kit::resolveUrl(qmlContext(this), QUrl(img));
    }
    m_items.push_back(std::move(e));
  }
  m_pressIndex = m_pressShown = -1;
  const QList<int> before = m_checked;
  structureChanged();
  emit modelChanged();
  if (before != m_checked)
    emit checkedIndicesChanged();
}

void MaterialList::setVariant(Variant v) {
  if (assign(m_variant, v, &MaterialList::variantChanged))
    structureChanged();
}

void MaterialList::setSelectionMode(SelectionMode v) {
  if (!assign(m_mode, v, &MaterialList::selectionModeChanged))
    return;
  if (v == NoSelection)
    m_checked.clear();
  else if (v == Single && m_checked.size() > 1)
    m_checked = {m_checked.first()};
  emit checkedIndicesChanged();
}

void MaterialList::setCheckedIndices(const QList<int> &v) {
  QList<int> c = v;
  std::sort(c.begin(), c.end());
  c.erase(std::unique(c.begin(), c.end()), c.end());
  c.erase(std::remove_if(c.begin(), c.end(),
                         [this](int i) { return !selectable(i); }),
          c.end());
  if (m_mode == NoSelection)
    c.clear();
  if (m_mode == Single && c.size() > 1)
    c = {c.first()};
  assign(m_checked, c, &MaterialList::checkedIndicesChanged);
}

void MaterialList::setCurrentIndex(int i) {
  setCheckedIndices(i < 0 ? QList<int>{} : QList<int>{i});
}

void MaterialList::setChecked(int i, bool on) {
  QList<int> c = m_checked;
  if (m_mode == Single)
    c = on ? QList<int>{i} : QList<int>{};
  else if (on && !c.contains(i))
    c.append(i);
  else if (!on)
    c.removeAll(i);
  setCheckedIndices(c);
}

void MaterialList::setFont(const QFont &v) {
  if (assign(m_font, v, &MaterialList::fontChanged))
    ++m_fontGen;
}

void MaterialList::setIconFont(const QFont &v) {
  assign(m_iconFont, v, &MaterialList::iconFontChanged);
}

void MaterialList::setItemColor(const QColor &v) {
  assign(m_itemColor, v, &MaterialList::itemColorChanged);
}

void MaterialList::setSelectedColor(const QColor &v) {
  assign(m_selectedColor, v, &MaterialList::selectedColorChanged);
}

void MaterialList::setTextColor(const QColor &v) {
  assign(m_textColor, v, &MaterialList::textColorChanged);
}

void MaterialList::setSelectedTextColor(const QColor &v) {
  assign(m_selectedTextColor, v, &MaterialList::selectedTextColorChanged);
}

void MaterialList::setSupportingColor(const QColor &v) {
  assign(m_supportingColor, v, &MaterialList::supportingColorChanged);
}

void MaterialList::setIconColor(const QColor &v) {
  assign(m_iconColor, v, &MaterialList::iconColorChanged);
}

void MaterialList::setAvatarColor(const QColor &v) {
  assign(m_avatarColor, v, &MaterialList::avatarColorChanged);
}

void MaterialList::setAvatarTextColor(const QColor &v) {
  assign(m_avatarTextColor, v, &MaterialList::avatarTextColorChanged);
}

void MaterialList::setDividerColor(const QColor &v) {
  assign(m_dividerColor, v, &MaterialList::dividerColorChanged);
}

void MaterialList::setEqualizerIndex(int v) {
  assign(m_eqIndex, v, &MaterialList::equalizerIndexChanged);
}

void MaterialList::setEqualizerLevels(const QList<qreal> &v) {
  assign(m_eqLevels, v, &MaterialList::equalizerLevelsChanged);
}

void MaterialList::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled()) {
    e->ignore();
    return;
  }
  const int i = rowAt(e->position());
  if (!selectable(i)) {
    e->ignore();
    return;
  }
  m_pressIndex = i;
  m_pressShown = i;
  m_press.go(1.0);
  animate();
  m_held = false;
  m_holdTimer.start();
  m_pressClock.restart();
  QTimer::singleShot(100, this, [this] { update(); });
  update();
  e->accept();
}

void MaterialList::mouseReleaseEvent(QMouseEvent *e) {
  m_holdTimer.stop();
  if (m_held) {
    m_held = false;
    e->accept();
    return;
  }
  if (m_pressIndex < 0)
    return;
  const int pressed = m_pressIndex;
  m_pressIndex = -1;
  m_press.go(0.0);
  animate();
  const bool inside = rowAt(e->position()) == pressed;
  update();
  if (inside) {
    m_flashIndex = pressed;
    QTimer::singleShot(120, this, [this] {
      m_flashIndex = -1;
      update();
    });
    activate(pressed);
  }
  e->accept();
}

void MaterialList::mouseUngrabEvent() {
  m_holdTimer.stop();
  m_held = false;
  if (m_pressIndex < 0)
    return;
  m_pressIndex = -1;
  m_press.go(0.0);
  animate();
  update();
}

void MaterialList::build(Kit::Mesh &m) {
  const int n = int(m_items.size());
  if (n == 0 || width() <= 0.0)
    return;
  int first = 0, last = -1;
  if (!visibleRows(first, last))
    return;
  last = std::min(last, first + kWindow - 1);
  const qreal dpr = m.dpr();
  const qreal fd = fade();
  const qreal w = width();
  const int thumbPx = std::max(1, qRound(kImage * dpr));
  const QFont headF = Kit::styledFont(m_font, 16.0, QFont::Normal);
  const QFont supF = Kit::styledFont(m_font, 14.0, QFont::Normal);
  const QFont ovF = Kit::styledFont(m_font, 11.0, QFont::Medium);
  const QFont avatarF = Kit::styledFont(m_font, 16.0, QFont::Medium);
  const QFontMetricsF headM(headF);
  const QFontMetricsF supM(supF);
  const QFontMetricsF ovM(ovF);
  const QFontMetricsF avatarM(avatarF);
  const QColor base = resolvedItemColor();
  const QColor iconBase =
      m_iconColor.isValid() ? m_iconColor : m_supportingColor;
  const bool slide = m_mode == Single;
  const qreal ha = std::clamp(m_hlA.x, 0.0, 1.0);
  if (slide && ha > 0.001)
    m.fillRoundedRect(
        QRectF(0.0, m_hlY.x, w, std::max(0.0, m_hlH.x)), kOuterR, kOuterR,
        kOuterR, kOuterR,
        Kit::withAlpha(m_selectedColor, m_selectedColor.alphaF() * ha));
  QSet<QString> wanted;
  for (int i = first; i <= last; ++i) {
    Entry &e = m_items[i];
    const QRectF r(0.0, m_tops[i], w, m_hts[i]);
    if (e.divider) {
      if (m_variant == Standard) {
        const qreal y = std::floor(r.center().y()) + 0.5;
        m.line(QPointF(r.left() + kPad, y), QPointF(r.right() - kPad, y),
               m.snapStroke(1.0), m_dividerColor);
      }
      continue;
    }
    const bool checked = isChecked(i);
    const qreal dim = e.enabled ? 1.0 : Kit::kDisabledOpacity;
    auto d = [dim](const QColor &c) {
      return Kit::withAlpha(c, c.alphaF() * dim);
    };
    qreal rt = kOuterR, rb = kOuterR;
    if (m_variant == Segmented) {
      const bool isFirst = i == 0 || m_items[i - 1].divider;
      const bool isLast = i == n - 1 || m_items[i + 1].divider;
      rt = isFirst ? kOuterR : kInnerR;
      rb = isLast ? kOuterR : kInnerR;
      if (m_pressShown == i) {
        const qreal t = std::clamp(m_press.x, 0.0, 1.0);
        rt += (kOuterR - rt) * t;
        rb += (kOuterR - rb) * t;
      }
    }
    const QColor bg = (checked && !slide) ? m_selectedColor : base;
    const QColor fg = checked ? m_selectedTextColor : m_textColor;
    const QColor sup = checked ? m_selectedTextColor : m_supportingColor;
    const QColor ic = checked ? m_selectedTextColor : iconBase;
    if (bg.alpha() > 0)
      m.fillRoundedRect(r, rt, rt, rb, rb, d(bg));
    if ((m_pressIndex == i && m_pressClock.elapsed() >= 100) ||
        m_flashIndex == i)
      m.fillRoundedRect(r, rt, rt, rb, rb,
                        d(Kit::withAlpha(fg, Kit::kPressLayer)));
    const int slot = (i % kWindow) * kSlots;
    const qreal cy = r.center().y();
    qreal xl = r.left() + kPad;
    qreal xr = r.right() - kPad;
    const bool hasImage = !e.imagePath.isEmpty();
    if (i == m_eqIndex) {
      const qreal S = hasImage ? kImage : kAvatar;
      const qreal rad = hasImage ? kImageR : S / 2.0;
      const QRectF box(xl, cy - S / 2.0, S, S);
      m.fillRoundedRect(box, rad, rad, rad, rad, d(m_avatarColor));
      const int nb = m_eqLevels.isEmpty() ? 4 : int(m_eqLevels.size());
      const qreal maxH = S * 0.45;
      const qreal total = nb * kEqBarW + (nb - 1) * kEqBarGap;
      const qreal baseY = cy + maxH / 2.0;
      qreal bx = box.center().x() - total / 2.0;
      for (int b = 0; b < nb; ++b) {
        const qreal lv =
            b < m_eqLevels.size() ? std::clamp(m_eqLevels[b], 0.0, 1.0) : 0.0;
        const qreal bh = kEqMinH + (maxH - kEqMinH) * lv;
        m.fillRoundedRect(QRectF(m.snap(bx), baseY - bh, kEqBarW, bh),
                          kEqBarW / 2.0, kEqBarW / 2.0, kEqBarW / 2.0,
                          kEqBarW / 2.0, d(m_avatarTextColor));
        bx += kEqBarW + kEqBarGap;
      }
      xl += S + kGap;
    } else if (hasImage) {
      const QString key =
          e.imagePath + QLatin1Char('@') + QString::number(thumbPx);
      wanted.insert(key);
      const QImage *img = thumbCache().object(key);
      const QRectF box(m.snap(xl), m.snap(cy - kImage / 2.0), kImage, kImage);
      if (img && !img->isNull()) {
        const qreal o = fd * dim;
        const QString lk =
            QStringList{"img", key, QString::number(o), QString::number(dpr)}
                .join(QLatin1Char('|'));
        useLayer(slot + kSlotImage, lk, box, [&]() -> QImage {
          if (o >= 0.999)
            return *img;
          QImage out = Kit::newImage(kImage, kImage, dpr);
          QPainter p(&out);
          p.setOpacity(o);
          p.drawImage(QPointF(0.0, 0.0), *img);
          p.end();
          return out;
        });
      } else {
        m.fillRoundedRect(QRectF(xl, cy - kImage / 2.0, kImage, kImage),
                          kImageR, kImageR, kImageR, kImageR, d(m_avatarColor));
        if (!img)
          requestThumb(key, e.imagePath, thumbPx, dpr);
      }
      xl += kImage + kGap;
    } else if (!e.avatarText.isEmpty()) {
      m.fillCircle(QPointF(xl + kAvatar / 2.0, cy), kAvatar / 2.0,
                   d(m_avatarColor));
      const qreal tw = avatarM.horizontalAdvance(e.avatarText);
      textLayer(slot + kSlotAvatar, avatarF, e.avatarText, d(m_avatarTextColor),
                xl + kAvatar / 2.0 - tw / 2.0, cy);
      xl += kAvatar + kGap;
    } else if (!e.leadingIcon.isEmpty()) {
      glyphLayer(slot + kSlotLeading, m_iconFont, e.leadingIcon, d(ic),
                 QPointF(xl + kIcon / 2.0, cy), kIcon);
      xl += kIcon + kGap;
    }
    bool hasTrailing = false;
    if (!e.trailingIcon.isEmpty()) {
      glyphLayer(slot + kSlotTrailingIcon, m_iconFont, e.trailingIcon, d(ic),
                 QPointF(xr - kIcon / 2.0, cy), kIcon);
      xr -= kIcon;
      hasTrailing = true;
    }
    if (!e.trailingText.isEmpty()) {
      if (hasTrailing)
        xr -= kGap;
      const qreal tw = ovM.horizontalAdvance(e.trailingText);
      textLayer(slot + kSlotTrailingText, ovF, e.trailingText, d(sup), xr - tw,
                cy);
      xr -= tw;
      hasTrailing = true;
    }
    if (hasTrailing)
      xr -= kGap;
    const qreal avail = std::max(0.0, xr - xl);
    if (e.eGen != m_fontGen || e.eAvail != avail) {
      e.eGen = m_fontGen;
      e.eAvail = avail;
      e.eOver = e.overline.isEmpty()
                    ? QString()
                    : ovM.elidedText(e.overline, Qt::ElideRight, avail);
      e.eHead = e.headline.isEmpty()
                    ? QString()
                    : headM.elidedText(e.headline, Qt::ElideRight, avail);
      e.eSup = e.supporting.isEmpty()
                   ? QString()
                   : supM.elidedText(e.supporting, Qt::ElideRight, avail);
    }
    struct Line {
      int slot;
      const QString *s;
      const QFont *f;
      qreal lh;
      QColor c;
    };
    Line lines[3];
    int ln = 0;
    if (!e.overline.isEmpty())
      lines[ln++] = {kSlotOverline, &e.eOver, &ovF, kOverlineLh, sup};
    if (!e.headline.isEmpty())
      lines[ln++] = {kSlotHeadline, &e.eHead, &headF, kHeadlineLh, fg};
    if (!e.supporting.isEmpty())
      lines[ln++] = {kSlotSupporting, &e.eSup, &supF, kSupportLh, sup};
    qreal total = 0.0;
    for (int k = 0; k < ln; ++k)
      total += lines[k].lh;
    qreal y = cy - total / 2.0;
    for (int k = 0; k < ln; ++k) {
      textLayer(slot + lines[k].slot, *lines[k].f, *lines[k].s, d(lines[k].c),
                xl, y + lines[k].lh / 2.0);
      y += lines[k].lh;
    }
  }
  QMutexLocker lock(&m_wanted->m);
  m_wanted->s = std::move(wanted);
}
