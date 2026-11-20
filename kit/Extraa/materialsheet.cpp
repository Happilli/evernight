#include "./materialsheet.hpp"
#include <algorithm>

namespace {
constexpr qreal kScrimAlpha = 0.32;
constexpr qreal kBottomRadius = 28.0;
constexpr qreal kSideRadius = 16.0;
constexpr qreal kSideWidth = 360.0;
constexpr qreal kSideMargin = 56.0;
constexpr qreal kMinVertical = 200.0;
constexpr qreal kHandleW = 32.0;
constexpr qreal kHandleH = 4.0;
constexpr qreal kHandleTop = 16.0;
constexpr qreal kHandleArea = 36.0;
constexpr qreal kDragSlop = 4.0;
constexpr qreal kFlingVelocity = 900.0;
constexpr qreal kDismissProgress = 0.6;
constexpr qreal kShownEps = 0.001;
constexpr qreal kStiffness = 380.0;
constexpr qreal kDamping = 1.0;
} // namespace

MaterialSheet::MaterialSheet(QQuickItem *parent)
    : SgMaterialItem(parent, kStiffness, kDamping),
      m_panel(new QQuickItem(this)) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setClip(true);
  m_panel->setClip(true);
  m_panel->setVisible(false);
}

QQmlListProperty<QObject> MaterialSheet::content() {
  return QQmlListProperty<QObject>(this, this, &MaterialSheet::appendContent,
                                   &MaterialSheet::contentCount,
                                   &MaterialSheet::contentAt, nullptr);
}

void MaterialSheet::appendContent(QQmlListProperty<QObject> *list, QObject *o) {
  auto *self = static_cast<MaterialSheet *>(list->data);
  if (auto *item = qobject_cast<QQuickItem *>(o))
    item->setParentItem(self->m_panel);
  else
    o->setParent(self);
}

qsizetype MaterialSheet::contentCount(QQmlListProperty<QObject> *list) {
  return static_cast<MaterialSheet *>(list->data)->m_panel->childItems().size();
}

QObject *MaterialSheet::contentAt(QQmlListProperty<QObject> *list,
                                  qsizetype i) {
  return static_cast<MaterialSheet *>(list->data)->m_panel->childItems().at(i);
}

void MaterialSheet::componentComplete() {
  SgMaterialItem::componentComplete();
  retarget(true);
}

void MaterialSheet::geometryChange(const QRectF &newGeometry,
                                   const QRectF &oldGeometry) {
  SgMaterialItem::geometryChange(newGeometry, oldGeometry);
  applyLayout();
  update();
}

bool MaterialSheet::vertical() const {
  return m_edge == FromBottom || m_edge == FromTop;
}

qreal MaterialSheet::extent() const {
  const qreal limit = vertical() ? height() : width();
  if (m_extent > 0.0)
    return std::min(m_extent, limit);
  if (vertical())
    return std::min(limit, std::max(kMinVertical, limit * 0.5));
  return std::max(0.0, std::min(kSideWidth, limit - kSideMargin));
}

qreal MaterialSheet::cornerRadius() const {
  if (m_radius >= 0.0)
    return m_radius;
  return vertical() ? kBottomRadius : kSideRadius;
}

QRectF MaterialSheet::sheetRect() const {
  const qreal w = width();
  const qreal h = height();
  const qreal e = extent();
  const qreal raw = std::clamp(m_prog.x, 0.0, 1.3);
  const qreal p = std::min(raw, 1.0);
  const qreal off = e * (1.0 - p);
  const qreal over = e * (raw - p);
  const qreal cw =
      (vertical() && m_maxCross > 0.0) ? std::min(w, m_maxCross) : w;
  const qreal x = (w - cw) / 2.0;
  switch (m_edge) {
  case FromBottom:
    return QRectF(x, h - e + off - over, cw, e + over);
  case FromTop:
    return QRectF(x, -off, cw, e + over);
  case FromLeft:
    return QRectF(-off, 0.0, e + over, h);
  case FromRight:
    return QRectF(w - e + off - over, 0.0, e + over, h);
  }
  return {};
}

QRectF MaterialSheet::contentRect(const QRectF &sheet) const {
  QRectF c = sheet;
  if (m_edge == FromBottom && m_showHandle)
    c.setTop(sheet.top() + kHandleArea);
  return c;
}

qreal MaterialSheet::dragDelta(const QPointF &p) const {
  switch (m_edge) {
  case FromBottom:
    return p.y() - m_pressPos.y();
  case FromTop:
    return m_pressPos.y() - p.y();
  case FromLeft:
    return m_pressPos.x() - p.x();
  case FromRight:
    return p.x() - m_pressPos.x();
  }
  return 0.0;
}

void MaterialSheet::applyLayout() {
  const bool shown = m_open || m_prog.x > kShownEps;
  m_panel->setVisible(shown);
  const QRectF c = contentRect(sheetRect());
  m_panel->setPosition(c.topLeft());
  m_panel->setSize(c.size());
}

void MaterialSheet::retarget(bool immediate) {
  m_prog.go(m_open ? 1.0 : 0.0, immediate);
  if (immediate) {
    m_settled = true;
    applyLayout();
    update();
    return;
  }
  animate();
}

void MaterialSheet::dismiss() {
  if (!m_open)
    return;
  setOpen(false);
  emit dismissed();
}

bool MaterialSheet::advance(qreal dt) {
  if (m_drag)
    return false;
  const bool active = step(m_prog, dt);
  applyLayout();
  if (!active && !m_settled) {
    m_settled = true;
    if (m_open)
      emit opened();
    else
      emit closed();
  }
  return active;
}

void MaterialSheet::setOpen(bool v) {
  if (!assign(m_open, v, &MaterialSheet::openChanged))
    return;
  m_settled = false;
  m_drag = false;
  retarget();
}

void MaterialSheet::setEdge(Edge v) {
  assign(m_edge, v, &MaterialSheet::edgeChanged);
}

void MaterialSheet::setModal(bool v) {
  assign(m_modal, v, &MaterialSheet::modalChanged);
}

void MaterialSheet::setDraggable(bool v) {
  assign(m_draggable, v, &MaterialSheet::draggableChanged);
}

void MaterialSheet::setShowHandle(bool v) {
  assign(m_showHandle, v, &MaterialSheet::showHandleChanged);
}

void MaterialSheet::setExtent(qreal v) {
  assign(m_extent, v, &MaterialSheet::extentChanged);
}

void MaterialSheet::setMaxCrossSize(qreal v) {
  assign(m_maxCross, v, &MaterialSheet::maxCrossSizeChanged);
}

void MaterialSheet::setRadius(qreal v) {
  assign(m_radius, v, &MaterialSheet::radiusChanged);
}

void MaterialSheet::setColor(const QColor &v) {
  assign(m_color, v, &MaterialSheet::colorChanged);
}

void MaterialSheet::setScrimColor(const QColor &v) {
  assign(m_scrimColor, v, &MaterialSheet::scrimColorChanged);
}

void MaterialSheet::setHandleColor(const QColor &v) {
  assign(m_handleColor, v, &MaterialSheet::handleColorChanged);
}

void MaterialSheet::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled() || !m_open) {
    e->ignore();
    return;
  }
  const QPointF p = e->position();
  if (sheetRect().contains(p)) {
    m_drag = m_draggable;
    m_moved = false;
    m_pressPos = p;
    m_dragBase = std::clamp(m_prog.x, 0.0, 1.0);
    m_lastDelta = 0.0;
    m_velocity = 0.0;
    m_clock.restart();
    e->accept();
    return;
  }
  if (m_modal) {
    m_scrimPress = true;
    e->accept();
    return;
  }
  e->ignore();
}

void MaterialSheet::mouseMoveEvent(QMouseEvent *e) {
  if (!m_drag) {
    e->ignore();
    return;
  }
  const qreal delta = dragDelta(e->position());
  if (!m_moved && delta < kDragSlop) {
    e->accept();
    return;
  }
  m_moved = true;
  const qreal dt = m_clock.restart() / 1000.0;
  if (dt > 0.0)
    m_velocity = 0.6 * m_velocity + 0.4 * ((delta - m_lastDelta) / dt);
  m_lastDelta = delta;
  m_prog.x = std::clamp(
      m_dragBase - std::max(delta, 0.0) / std::max(extent(), 1.0), 0.0, 1.0);
  m_prog.v = 0.0;
  applyLayout();
  update();
  e->accept();
}

void MaterialSheet::mouseReleaseEvent(QMouseEvent *e) {
  if (m_scrimPress) {
    m_scrimPress = false;
    if (!sheetRect().contains(e->position()))
      dismiss();
    e->accept();
    return;
  }
  if (!m_drag) {
    e->ignore();
    return;
  }
  m_drag = false;
  const qreal v = m_clock.elapsed() > 100 ? 0.0 : m_velocity;
  const bool close =
      m_moved && (v > kFlingVelocity || m_prog.x < kDismissProgress);
  if (close)
    dismiss();
  else
    retarget();
  e->accept();
}

void MaterialSheet::mouseUngrabEvent() {
  m_scrimPress = false;
  if (!m_drag)
    return;
  m_drag = false;
  retarget();
}

void MaterialSheet::build(Kit::Mesh &m) {
  const qreal w = width();
  const qreal h = height();
  const qreal p = std::clamp(m_prog.x, 0.0, 1.0);
  if (w <= 0.0 || h <= 0.0 || p <= 0.0)
    return;

  const QColor container = m_color.isValid() ? m_color : QColor("#F7F2FA");
  const QColor scrim =
      m_scrimColor.isValid() ? m_scrimColor : QColor(Qt::black);
  QColor handle = m_handleColor.isValid() ? m_handleColor : QColor("#49454F");
  if (!m_handleColor.isValid())
    handle.setAlphaF(0.4);

  if (m_modal)
    m.fillRoundedRect(QRectF(0.0, 0.0, w, h), 0.0, 0.0, 0.0, 0.0,
                      Kit::withAlpha(scrim, kScrimAlpha * p));

  const QRectF r = sheetRect();
  const QRectF sr(QPointF(m.snap(r.left()), m.snap(r.top())),
                  QPointF(m.snap(r.right()), m.snap(r.bottom())));
  const qreal rad = cornerRadius();
  qreal tl = 0.0, tr = 0.0, br = 0.0, bl = 0.0;
  switch (m_edge) {
  case FromBottom:
    tl = tr = rad;
    break;
  case FromTop:
    br = bl = rad;
    break;
  case FromLeft:
    tr = br = rad;
    break;
  case FromRight:
    tl = bl = rad;
    break;
  }
  m.fillRoundedRect(sr, tl, tr, br, bl, container);

  if (m_edge == FromBottom && m_showHandle) {
    const QRectF hr(m.snap(sr.center().x() - kHandleW / 2.0),
                    m.snap(sr.top() + kHandleTop), kHandleW, kHandleH);
    m.fillRoundedRect(hr, kHandleH / 2.0, kHandleH / 2.0, kHandleH / 2.0,
                      kHandleH / 2.0, handle);
  }
}
