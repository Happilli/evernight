#include "./materialtabs.hpp"
#include "../Core/paintutil.hpp"
#include <QFontMetricsF>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kHeightText = 48.0;
constexpr qreal kHeightIcon = 64.0;
constexpr qreal kPad = 16.0;
constexpr qreal kMinTab = 90.0;
constexpr qreal kIcon = 24.0;
constexpr qreal kGap = 8.0;
constexpr qreal kFontPx = 14.0;
constexpr qreal kPrimaryInd = 3.0;
constexpr qreal kSecondaryInd = 2.0;
constexpr qreal kMinInd = 24.0;
constexpr qreal kStackIconCy = 24.0;
constexpr qreal kStackTextCy = 47.0;
constexpr qreal kSelStiffness = 1.0;
constexpr qreal kFastEdge = 1.8;
constexpr qreal kIndEps = 0.05;
} // namespace

MaterialTabs::MaterialTabs(QQuickItem *parent)
    : SgMaterialItem(parent, 520.0, 0.6) {
  setAcceptedMouseButtons(Qt::LeftButton);
  updateImplicit();
}

void MaterialTabs::componentComplete() {
  SgMaterialItem::componentComplete();
  updateImplicit();
  retarget(true);
  update();
}

void MaterialTabs::geometryChange(const QRectF &newGeometry,
                                  const QRectF &oldGeometry) {
  SgMaterialItem::geometryChange(newGeometry, oldGeometry);
  if (newGeometry.width() != oldGeometry.width())
    retarget(true);
}

QFont MaterialTabs::labelFont() const {
  return Kit::styledFont(m_font, kFontPx, m_fontWeight);
}

bool MaterialTabs::hasIconAt(int i) const {
  return i >= 0 && i < m_icons.size() && !m_icons.at(i).isEmpty();
}

qreal MaterialTabs::contentWidth(int i) const {
  const qreal tw = m_textW[i];
  if (!hasIconAt(i))
    return tw;
  if (m_type == Primary)
    return std::max(tw, kIcon);
  return kIcon + (tw > 0.0 ? kGap : 0.0) + tw;
}

qreal MaterialTabs::tabWidth(int i) const {
  const int n = count();
  if (i < 0 || i >= n)
    return 0.0;
  if (m_fixed)
    return width() / n;
  return m_natural[i];
}

qreal MaterialTabs::tabLeft(int i) const {
  if (m_fixed)
    return i * tabWidth(0);
  qreal x = 0.0;
  for (int k = 0; k < i; ++k)
    x += m_natural[k];
  return x;
}

qreal MaterialTabs::barHeight() const {
  if (m_type == Primary) {
    for (int i = 0; i < count(); ++i)
      if (hasIconAt(i))
        return kHeightIcon;
  }
  return kHeightText;
}

int MaterialTabs::tabAt(const QPointF &pt) const {
  if (pt.y() < 0.0 || pt.y() > height() || pt.x() < 0.0)
    return -1;
  for (int i = 0; i < count(); ++i) {
    const qreal x = tabLeft(i);
    if (pt.x() >= x && pt.x() < x + tabWidth(i))
      return i;
  }
  return -1;
}

void MaterialTabs::syncCount() {
  const size_t n = size_t(count());
  m_textW.resize(n, 0.0);
  m_natural.resize(n, 0.0);
  m_sel.resize(n);
}

void MaterialTabs::updateImplicit() {
  syncCount();
  const QFontMetricsF fm(labelFont());
  qreal total = 0.0;
  for (int i = 0; i < count(); ++i) {
    m_textW[i] =
        m_labels.at(i).isEmpty() ? 0.0 : fm.horizontalAdvance(m_labels.at(i));
    m_natural[i] = std::max(kMinTab, contentWidth(i) + 2.0 * kPad);
    total += m_natural[i];
  }
  setImplicitWidth(std::ceil(total));
  setImplicitHeight(barHeight());
}

void MaterialTabs::retarget(bool immediate) {
  const int n = count();
  syncCount();
  for (int i = 0; i < n; ++i)
    m_sel[i].go(i == m_current ? 1.0 : 0.0, immediate);
  if (m_current >= 0 && m_current < n && width() > 0.0) {
    const qreal x0 = tabLeft(m_current);
    const qreal tw = tabWidth(m_current);
    qreal l = x0;
    qreal r = x0 + tw;
    if (m_type == Primary) {
      const qreal cw = std::max(contentWidth(m_current), kMinInd);
      l = x0 + (tw - cw) / 2.0;
      r = l + cw;
    }
    const bool snap = immediate || !m_indReady;
    if (!snap) {
      m_scaleL = l > m_indL.t ? 1.0 : kFastEdge;
      m_scaleR = r > m_indR.t ? kFastEdge : 1.0;
    }
    m_indL.go(l, snap);
    m_indR.go(r, snap);
    m_indReady = true;
  }
  if (!immediate)
    animate();
}

bool MaterialTabs::advance(qreal dt) {
  bool active = false;
  for (size_t i = 0; i < m_sel.size(); ++i)
    active |= step(m_sel[i], dt, 0.001, kSelStiffness);
  active |= step(m_indL, dt, kIndEps, m_scaleL);
  active |= step(m_indR, dt, kIndEps, m_scaleR);
  return active;
}

void MaterialTabs::setLabels(const QStringList &v) {
  const int before = count();
  if (assign(m_labels, v, &MaterialTabs::labelsChanged)) {
    updateImplicit();
    retarget(true);
    if (before != count())
      emit countChanged();
  }
}

void MaterialTabs::setIcons(const QStringList &v) {
  if (assign(m_icons, v, &MaterialTabs::iconsChanged)) {
    updateImplicit();
    retarget(true);
  }
}

void MaterialTabs::setCurrentIndex(int v) {
  v = std::max(-1, v);
  if (assign(m_current, v, &MaterialTabs::currentIndexChanged))
    retarget();
}

void MaterialTabs::setType(Type v) {
  if (assign(m_type, v, &MaterialTabs::typeChanged)) {
    updateImplicit();
    retarget(true);
  }
}

void MaterialTabs::setFixed(bool v) {
  if (assign(m_fixed, v, &MaterialTabs::fixedChanged))
    retarget(true);
}

void MaterialTabs::setShowDivider(bool v) {
  assign(m_showDivider, v, &MaterialTabs::showDividerChanged);
}

void MaterialTabs::setFont(const QFont &v) {
  if (assign(m_font, v, &MaterialTabs::fontChanged)) {
    updateImplicit();
    retarget(true);
  }
}

void MaterialTabs::setFontWeight(int v) {
  v = qBound(1, v, 1000);
  if (assign(m_fontWeight, v, &MaterialTabs::fontWeightChanged)) {
    updateImplicit();
    retarget(true);
  }
}

void MaterialTabs::setIconFont(const QFont &v) {
  assign(m_iconFont, v, &MaterialTabs::iconFontChanged);
}

void MaterialTabs::setColor(const QColor &v) {
  assign(m_color, v, &MaterialTabs::colorChanged);
}

void MaterialTabs::setActiveColor(const QColor &v) {
  assign(m_activeColor, v, &MaterialTabs::activeColorChanged);
}

void MaterialTabs::setInactiveColor(const QColor &v) {
  assign(m_inactiveColor, v, &MaterialTabs::inactiveColorChanged);
}

void MaterialTabs::setIndicatorColor(const QColor &v) {
  assign(m_indicatorColor, v, &MaterialTabs::indicatorColorChanged);
}

void MaterialTabs::setDividerColor(const QColor &v) {
  assign(m_dividerColor, v, &MaterialTabs::dividerColorChanged);
}

void MaterialTabs::setPress(int i) {
  if (m_pressIdx == i)
    return;
  m_pressIdx = i;
  update();
}

void MaterialTabs::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled()) {
    e->ignore();
    return;
  }
  const int i = tabAt(e->position());
  if (i < 0) {
    e->ignore();
    return;
  }
  setPress(i);
  e->accept();
}

void MaterialTabs::mouseReleaseEvent(QMouseEvent *e) {
  const int pressed = m_pressIdx;
  if (pressed < 0)
    return;
  const bool inside = tabAt(e->position()) == pressed;
  setPress(-1);
  if (inside) {
    setCurrentIndex(pressed);
    emit tabClicked(pressed);
  }
  e->accept();
}

void MaterialTabs::mouseUngrabEvent() { setPress(-1); }

void MaterialTabs::build(Kit::Mesh &m) {
  const qreal w = width();
  const qreal h = height();
  const int n = count();
  if (w <= 0.0 || h <= 0.0)
    return;

  const bool primary = m_type == Primary;
  const QColor active = m_activeColor.isValid()
                            ? m_activeColor
                            : (primary ? QColor("#6750A4") : QColor("#1D1B20"));
  const QColor inactive =
      m_inactiveColor.isValid() ? m_inactiveColor : QColor("#49454F");
  const QColor indicator =
      m_indicatorColor.isValid() ? m_indicatorColor : QColor("#6750A4");
  const QColor divider =
      m_dividerColor.isValid() ? m_dividerColor : QColor("#CAC4D0");

  if (m_color.isValid() && m_color.alpha() > 0)
    m.fillRoundedRect(QRectF(0.0, 0.0, w, h), 0.0, 0.0, 0.0, 0.0, m_color);

  if (m_showDivider) {
    const qreal t = m.snapStroke(1.0);
    m.fillRoundedRect(QRectF(0.0, h - t, w, t), 0.0, 0.0, 0.0, 0.0, divider);
  }

  const QFont lf = labelFont();
  const qreal indH = primary ? kPrimaryInd : kSecondaryInd;
  const qreal bodyH = h - indH;

  for (int i = 0; i < n; ++i) {
    const qreal x0 = tabLeft(i);
    const qreal tw = tabWidth(i);
    const qreal sel = std::clamp(m_sel[i].x, 0.0, 1.0);
    const QColor fg = Kit::mix(inactive, active, sel);

    const qreal la = Kit::stateLayer(m_pressIdx == i);
    if (la > 0.0)
      m.fillRoundedRect(QRectF(x0, 0.0, tw, bodyH), 0.0, 0.0, 0.0, 0.0,
                        Kit::withAlpha(fg, la));

    const qreal cx = x0 + tw / 2.0;
    const qreal textW = m_textW[i];
    const bool icon = hasIconAt(i);
    if (icon && primary) {
      glyphLayer(i * 2 + 1, m_iconFont, m_icons.at(i), fg,
                 QPointF(cx, kStackIconCy), kIcon);
      textLayer(i * 2, lf, m_labels.at(i), fg, cx - textW / 2.0, kStackTextCy);
    } else if (icon) {
      const qreal cw = contentWidth(i);
      const qreal sx = cx - cw / 2.0;
      glyphLayer(i * 2 + 1, m_iconFont, m_icons.at(i), fg,
                 QPointF(sx + kIcon / 2.0, bodyH / 2.0), kIcon);
      textLayer(i * 2, lf, m_labels.at(i), fg, sx + kIcon + kGap, bodyH / 2.0);
    } else {
      textLayer(i * 2, lf, m_labels.at(i), fg, cx - textW / 2.0, bodyH / 2.0);
    }
  }

  if (m_current >= 0 && m_current < n && m_indReady) {
    const qreal l = m.snap(m_indL.x);
    const qreal r = m.snap(std::max(m_indR.x, m_indL.x));
    if (r > l) {
      const QRectF ir(l, h - indH, r - l, indH);
      if (primary)
        m.fillRoundedRect(ir, indH, indH, 0.0, 0.0, indicator);
      else
        m.fillRoundedRect(ir, 0.0, 0.0, 0.0, 0.0, indicator);
    }
  }
}
