#include "./materialtextfield.hpp"
#include "../Core/paintutil.hpp"
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QInputMethod>
#include <QMetaObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStringList>
#include <QTouchEvent>
#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kFieldH = 56.0;
constexpr qreal kMinW = 210.0;
constexpr qreal kRadius = 4.0;
constexpr qreal kPad = 16.0;
constexpr qreal kPadIcon = 52.0;
constexpr qreal kIcon = 24.0;
constexpr qreal kIconCx = 24.0;
constexpr qreal kTrailHit = 48.0;
constexpr qreal kTrailLayerR = 20.0;
constexpr qreal kFontPx = 16.0;
constexpr qreal kSmallPx = 12.0;
constexpr qreal kOutlinedTop = 8.0;
constexpr qreal kSupportH = 20.0;
constexpr qreal kInputH = 24.0;
constexpr qreal kFilledLabelCy = 16.0;
constexpr qreal kFilledTextCy = 37.0;
constexpr qreal kMidCy = 28.0;
constexpr qreal kFocusStiffness = 1.2;
constexpr int kArcSteps = 6;

constexpr int kSlotLead = 0;
constexpr int kSlotTrail = 1;
constexpr int kSlotLabel = 2;
constexpr int kSlotPlaceholder = 4;
constexpr int kSlotSupport = 5;

constexpr const char *kInputQml = R"QML(
import QtQuick
TextInput {
  id: ti
  property color accent: "#6750A4"
  property bool blink: false
  clip: true
  autoScroll: true
  verticalAlignment: TextInput.AlignVCenter
  cursorDelegate: Rectangle {
    id: cur
    property bool on: true
    width: 2
    radius: 1
    color: ti.accent
    opacity: (ti.activeFocus && on) ? 1 : 0
    Timer {
      interval: 530
      repeat: true
      running: ti.blink && ti.activeFocus
      onRunningChanged: cur.on = true
      onTriggered: cur.on = !cur.on
    }
  }
}
)QML";

qreal lerp(qreal a, qreal b, qreal t) { return a + (b - a) * t; }

} // namespace

MaterialTextField::MaterialTextField(QQuickItem *parent)
    : SgMaterialItem(parent, 520.0, 0.5) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setFlag(ItemIsFocusScope, true);
  updateImplicit();
  retarget(true);
}

MaterialTextField::~MaterialTextField() { detachFilter(); }

void MaterialTextField::componentComplete() {
  SgMaterialItem::componentComplete();
  createInput();
  updateImplicit();
  retarget(true);
  update();
}

void MaterialTextField::createInput() {
  if (m_input)
    return;
  QQmlEngine *engine = qmlEngine(this);
  if (!engine)
    return;
  QQmlComponent comp(engine);
  comp.setData(kInputQml, QUrl());
  QQmlContext *ctx =
      qmlContext(this) ? qmlContext(this) : engine->rootContext();
  QObject *obj = comp.create(ctx);
  m_input = qobject_cast<QQuickItem *>(obj);
  if (!m_input) {
    qWarning("MaterialTextField: input creation failed: %s",
             qPrintable(comp.errorString()));
    delete obj;
    return;
  }
  m_input->setParent(this);
  m_input->setParentItem(this);
  m_input->setFocus(true);
  connect(m_input, SIGNAL(textChanged()), this, SLOT(onInputText()));
  connect(m_input, SIGNAL(textEdited()), this, SLOT(onInputEdited()));
  connect(m_input, SIGNAL(accepted()), this, SLOT(onInputAccepted()));
  connect(m_input, SIGNAL(cursorPositionChanged()), this,
          SLOT(onInputCursor()));
  connect(m_input, SIGNAL(selectedTextChanged()), this,
          SLOT(onInputSelection()));
  connect(m_input, SIGNAL(displayTextChanged()), this, SLOT(update()));
  connect(m_input, &QQuickItem::activeFocusChanged, this,
          &MaterialTextField::onInputFocus);
  m_input->setProperty("text", m_text);
  applyStyle();
  layoutInput();
}

void MaterialTextField::applyStyle() {
  if (!m_input)
    return;
  const QColor on = onSurfaceCol();
  m_input->setProperty("color", on);
  m_input->setProperty("selectedTextColor", on);
  m_input->setProperty("selectionColor",
                       m_selectionColor.isValid()
                           ? m_selectionColor
                           : Kit::withAlpha(accentCol(), 0.3));
  m_input->setProperty("accent", accentCol());
  m_input->setProperty("blink", m_cursorBlink);
  m_input->setProperty("readOnly", m_readOnly);
  m_input->setProperty("echoMode", m_echo == Password ? 2 : 0);
  m_input->setProperty("maximumLength", m_maxLen >= 0 ? m_maxLen : 32767);
  m_input->setProperty("font", textFont());
}

void MaterialTextField::layoutInput() {
  if (!m_input)
    return;
  m_input->setX(padLeft());
  m_input->setY(textCenter() - kInputH / 2.0);
  m_input->setWidth(viewWidth());
  m_input->setHeight(kInputH);
}

void MaterialTextField::propertyAssigned() {
  applyStyle();
  layoutInput();
}

void MaterialTextField::geometryChange(const QRectF &n, const QRectF &o) {
  SgMaterialItem::geometryChange(n, o);
  if (n.size() != o.size())
    layoutInput();
}

void MaterialTextField::itemChange(ItemChange change,
                                   const ItemChangeData &data) {
  SgMaterialItem::itemChange(change, data);
  if (change == ItemSceneChange) {
    detachFilter();
    if (data.window && m_focused)
      attachFilter();
  } else if (change == ItemVisibleHasChanged && !data.boolValue && m_focused) {
    blur();
  }
}

void MaterialTextField::attachFilter() {
  QQuickWindow *w = window();
  if (!w || m_filtered == w)
    return;
  detachFilter();
  w->installEventFilter(this);
  m_filtered = w;
}

void MaterialTextField::detachFilter() {
  if (m_filtered)
    m_filtered->removeEventFilter(this);
  m_filtered = nullptr;
}

bool MaterialTextField::eventFilter(QObject *watched, QEvent *event) {
  if (!m_focused || watched != m_filtered)
    return QObject::eventFilter(watched, event);
  QPointF scene;
  switch (event->type()) {
  case QEvent::MouseButtonPress:
    scene = static_cast<QMouseEvent *>(event)->scenePosition();
    break;
  case QEvent::TouchBegin: {
    const auto &points = static_cast<QTouchEvent *>(event)->points();
    if (points.isEmpty())
      return false;
    scene = points.constFirst().scenePosition();
    break;
  }
  default:
    return false;
  }
  if (!boundingRect().contains(mapFromScene(scene)))
    blur();
  return false;
}

void MaterialTextField::onInputFocus(bool focused) {
  if (m_focused == focused)
    return;
  m_focused = focused;
  if (focused) {
    attachFilter();
  } else {
    detachFilter();
    if (m_input)
      QMetaObject::invokeMethod(m_input, "deselect");
  }
  retarget();
  update();
}

void MaterialTextField::focusInput(Qt::FocusReason reason) {
  if (!m_input)
    return;
  m_input->setFocus(true);
  if (!hasActiveFocus())
    forceActiveFocus(reason);
}

qreal MaterialTextField::fieldTop() const {
  return (m_type == Outlined && !m_label.isEmpty()) ? kOutlinedTop : 0.0;
}

bool MaterialTextField::hasLeading() const {
  return !m_leadText.isEmpty() ||
         (m_leadSource.isValid() && !m_leadSource.isEmpty());
}

bool MaterialTextField::hasTrailing() const {
  return !m_trailText.isEmpty() ||
         (m_trailSource.isValid() && !m_trailSource.isEmpty());
}

bool MaterialTextField::composing() const {
  return m_input && !m_input->property("displayText").toString().isEmpty();
}

qreal MaterialTextField::padLeft() const {
  return hasLeading() ? kPadIcon : kPad;
}

qreal MaterialTextField::padRight() const {
  return hasTrailing() ? kPadIcon : kPad;
}

qreal MaterialTextField::viewWidth() const {
  return std::max(0.0, width() - padLeft() - padRight());
}

qreal MaterialTextField::textCenter() const {
  const qreal top = fieldTop();
  if (!m_label.isEmpty() && m_type == Filled)
    return top + kFilledTextCy;
  return top + kMidCy;
}

QFont MaterialTextField::textFont() const {
  return Kit::styledFont(m_font, kFontPx, m_fontWeight);
}

QColor MaterialTextField::onSurfaceCol() const {
  return m_contentColor.isValid() ? m_contentColor : QColor("#1D1B20");
}

QColor MaterialTextField::variantCol() const {
  return m_labelColor.isValid() ? m_labelColor : QColor("#49454F");
}

QColor MaterialTextField::accentCol() const {
  return m_accentColor.isValid() ? m_accentColor : QColor("#6750A4");
}

QColor MaterialTextField::errorCol() const {
  return m_errorColor.isValid() ? m_errorColor : QColor("#B3261E");
}

QColor MaterialTextField::outlineCol() const {
  return m_outlineColor.isValid() ? m_outlineColor : QColor("#79747E");
}

QColor MaterialTextField::containerCol() const {
  return m_color.isValid() ? m_color : QColor("#E6E0E9");
}

int MaterialTextField::cursorPosition() const {
  return m_input ? m_input->property("cursorPosition").toInt() : 0;
}

QString MaterialTextField::selectedText() const {
  return m_input ? m_input->property("selectedText").toString() : QString();
}

void MaterialTextField::onInputText() {
  const QString t = m_input->property("text").toString();
  if (t == m_text) {
    update();
    return;
  }
  m_text = t;
  emit textChanged();
  retarget();
  update();
}

void MaterialTextField::onInputEdited() { emit textEdited(); }
void MaterialTextField::onInputAccepted() { emit accepted(); }
void MaterialTextField::onInputCursor() { emit cursorPositionChanged(); }
void MaterialTextField::onInputSelection() { emit selectionChanged(); }

void MaterialTextField::updateImplicit() {
  setImplicitWidth(kMinW);
  setImplicitHeight(fieldTop() + kFieldH +
                    (m_support.isEmpty() ? 0.0 : kSupportH));
}

void MaterialTextField::retarget(bool immediate) {
  m_float.go(m_focused || !m_text.isEmpty() ? 1.0 : 0.0, immediate);
  m_focus.go(m_focused ? 1.0 : 0.0, immediate);
  if (!immediate)
    animate();
}

bool MaterialTextField::advance(qreal dt) {
  bool active = step(m_float, dt);
  active |= step(m_focus, dt, 0.001, kFocusStiffness);
  return active;
}

void MaterialTextField::setText(const QString &v) {
  QString t = v;
  if (m_maxLen >= 0)
    t.truncate(m_maxLen);
  if (m_input) {
    m_input->setProperty("text", t);
    return;
  }
  if (m_text == t)
    return;
  m_text = t;
  emit textChanged();
  retarget();
  update();
}

void MaterialTextField::setLabel(const QString &v) {
  if (assign(m_label, v, &MaterialTextField::labelChanged))
    updateImplicit();
}

void MaterialTextField::setPlaceholderText(const QString &v) {
  assign(m_placeholder, v, &MaterialTextField::placeholderTextChanged);
}

void MaterialTextField::setSupportingText(const QString &v) {
  if (assign(m_support, v, &MaterialTextField::supportingTextChanged))
    updateImplicit();
}

void MaterialTextField::setType(Type v) {
  if (assign(m_type, v, &MaterialTextField::typeChanged))
    updateImplicit();
}

void MaterialTextField::setEchoMode(EchoMode v) {
  assign(m_echo, v, &MaterialTextField::echoModeChanged);
}

void MaterialTextField::setError(bool v) {
  assign(m_error, v, &MaterialTextField::errorChanged);
}

void MaterialTextField::setReadOnly(bool v) {
  assign(m_readOnly, v, &MaterialTextField::readOnlyChanged);
}

void MaterialTextField::setCursorBlink(bool v) {
  assign(m_cursorBlink, v, &MaterialTextField::cursorBlinkChanged);
}

void MaterialTextField::setMaximumLength(int v) {
  if (v < 0)
    v = -1;
  if (assign(m_maxLen, v, &MaterialTextField::maximumLengthChanged) &&
      m_maxLen >= 0 && m_text.size() > m_maxLen)
    setText(m_text.left(m_maxLen));
}

void MaterialTextField::setCursorPosition(int v) {
  if (m_input)
    m_input->setProperty("cursorPosition", v);
}

void MaterialTextField::setFont(const QFont &v) {
  assign(m_font, v, &MaterialTextField::fontChanged);
}

void MaterialTextField::setFontWeight(int v) {
  v = qBound(1, v, 1000);
  assign(m_fontWeight, v, &MaterialTextField::fontWeightChanged);
}

void MaterialTextField::setLeadingIconText(const QString &v) {
  assign(m_leadText, v, &MaterialTextField::leadingIconTextChanged);
}

void MaterialTextField::setLeadingIconSource(const QUrl &v) {
  if (assign(m_leadSource, v, &MaterialTextField::leadingIconSourceChanged)) {
    m_leadPath.clear();
    m_leadCache.invalidate();
  }
}

void MaterialTextField::setTrailingIconText(const QString &v) {
  assign(m_trailText, v, &MaterialTextField::trailingIconTextChanged);
}

void MaterialTextField::setTrailingIconSource(const QUrl &v) {
  if (assign(m_trailSource, v, &MaterialTextField::trailingIconSourceChanged)) {
    m_trailPath.clear();
    m_trailCache.invalidate();
  }
}

void MaterialTextField::setIconFont(const QFont &v) {
  assign(m_iconFont, v, &MaterialTextField::iconFontChanged);
}

void MaterialTextField::setColor(const QColor &v) {
  assign(m_color, v, &MaterialTextField::colorChanged);
}

void MaterialTextField::setContentColor(const QColor &v) {
  assign(m_contentColor, v, &MaterialTextField::contentColorChanged);
}

void MaterialTextField::setLabelColor(const QColor &v) {
  assign(m_labelColor, v, &MaterialTextField::labelColorChanged);
}

void MaterialTextField::setAccentColor(const QColor &v) {
  assign(m_accentColor, v, &MaterialTextField::accentColorChanged);
}

void MaterialTextField::setErrorColor(const QColor &v) {
  assign(m_errorColor, v, &MaterialTextField::errorColorChanged);
}

void MaterialTextField::setOutlineColor(const QColor &v) {
  assign(m_outlineColor, v, &MaterialTextField::outlineColorChanged);
}

void MaterialTextField::setSelectionColor(const QColor &v) {
  assign(m_selectionColor, v, &MaterialTextField::selectionColorChanged);
}

void MaterialTextField::selectAll() {
  if (m_input)
    QMetaObject::invokeMethod(m_input, "selectAll");
}

void MaterialTextField::clear() { setText(QString()); }

void MaterialTextField::copy() {
  if (m_input && m_echo != Password)
    QMetaObject::invokeMethod(m_input, "copy");
}

void MaterialTextField::cut() {
  if (m_input && !m_readOnly && m_echo != Password)
    QMetaObject::invokeMethod(m_input, "cut");
}

void MaterialTextField::paste() {
  if (m_input && !m_readOnly)
    QMetaObject::invokeMethod(m_input, "paste");
}

void MaterialTextField::blur() {
  if (m_input)
    m_input->setFocus(false);
  setFocus(false);
  QGuiApplication::inputMethod()->hide();
}

bool MaterialTextField::inField(const QPointF &pt) const {
  const qreal top = fieldTop();
  return pt.x() >= 0.0 && pt.x() <= width() && pt.y() >= top &&
         pt.y() <= top + kFieldH;
}

bool MaterialTextField::overTrailing(const QPointF &pt) const {
  return hasTrailing() && inField(pt) && pt.x() >= width() - kTrailHit;
}

void MaterialTextField::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled() || !inField(e->position())) {
    e->ignore();
    return;
  }
  focusInput(Qt::MouseFocusReason);
  if (overTrailing(e->position())) {
    m_trailPress = true;
    update();
  }
  e->accept();
}

void MaterialTextField::mouseReleaseEvent(QMouseEvent *e) {
  const bool wasTrail = m_trailPress;
  m_trailPress = false;
  if (wasTrail) {
    if (overTrailing(e->position()))
      emit trailingClicked();
    update();
  } else if (!m_readOnly && isEnabled()) {
    QGuiApplication::inputMethod()->show();
  }
  e->accept();
}

void MaterialTextField::mouseUngrabEvent() {
  if (m_trailPress) {
    m_trailPress = false;
    update();
  }
}

void MaterialTextField::iconLayer(Kit::Mesh &m, int slot, Kit::IconCache &cache,
                                  QString &path, const QUrl &src,
                                  const QString &glyph, const QColor &col,
                                  const QPointF &c) {
  if (src.isValid() && !src.isEmpty()) {
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
  } else {
    glyphLayer(slot, m_iconFont, glyph, col, c, kIcon);
  }
}

void MaterialTextField::build(Kit::Mesh &m) {
  const qreal w = width();
  const qreal ih = height();
  if (w <= 0.0 || ih <= 0.0)
    return;

  const qreal top = fieldTop();
  const QRectF rect(0.0, top, w, kFieldH);
  const qreal fl = std::clamp(m_float.x, 0.0, 1.0);
  const qreal fo = std::clamp(m_focus.x, 0.0, 1.0);
  const bool filled = m_type == Filled;
  const bool hasLabel = !m_label.isEmpty();

  const QColor variant = variantCol();
  const QColor primary = accentCol();
  const QColor err = errorCol();
  const QColor outline = outlineCol();
  const QColor container = containerCol();

  const QColor idle = filled ? variant : outline;
  const QColor line = m_error ? err : Kit::mix(idle, primary, fo);
  const QColor labelCol = m_error ? err : Kit::mix(variant, primary, fo);
  const qreal lineW = lerp(1.0, 2.0, fo);

  const qreal pl = padLeft();
  const qreal labelFloatX = filled ? pl : kPad;
  const QFont floatFont = Kit::styledFont(m_font, kSmallPx, m_fontWeight);
  const qreal floatLabelW =
      hasLabel ? QFontMetricsF(floatFont).horizontalAdvance(m_label) : 0.0;

  if (filled) {
    m.fillRoundedRect(rect, kRadius, kRadius, 0.0, 0.0, container);
    const qreal t = m.snapStroke(lineW);
    m.fillRoundedRect(QRectF(0.0, top + kFieldH - t, w, t), 0.0, 0.0, 0.0, 0.0,
                      line);
  } else {
    const qreal bw = m.snapStroke(lineW);
    const qreal h = bw / 2.0;
    const qreal gapStart = labelFloatX - 4.0 - h;
    const qreal gapEnd = gapStart + (floatLabelW + 8.0) * fl + 2.0 * h;
    const qreal rad = std::max(0.0, kRadius - h);
    const QRectF r = rect.adjusted(h, h, -h, -h);
    if (!hasLabel || fl * (floatLabelW + 8.0) < 0.5 ||
        gapStart < r.left() + rad || gapEnd > r.right() - rad) {
      m.strokeRoundedRect(rect, kRadius, kRadius, kRadius, kRadius, bw, line);
    } else {
      QList<QPointF> pts;
      auto arc = [&](const QPointF &c, qreal a0) {
        for (int i = 0; i <= kArcSteps; ++i) {
          const qreal a = a0 + (M_PI / 2.0) * i / kArcSteps;
          pts.append(c + QPointF(std::cos(a), std::sin(a)) * rad);
        }
      };
      pts.append(QPointF(gapEnd, r.top()));
      arc(QPointF(r.right() - rad, r.top() + rad), -M_PI / 2.0);
      arc(QPointF(r.right() - rad, r.bottom() - rad), 0.0);
      arc(QPointF(r.left() + rad, r.bottom() - rad), M_PI / 2.0);
      arc(QPointF(r.left() + rad, r.top() + rad), M_PI);
      pts.append(QPointF(gapStart, r.top()));
      m.polyline(pts, bw, line);
    }
  }

  if (hasLabel) {
    const qreal px = lerp(kFontPx, kSmallPx, fl);
    const qreal restCy = top + kMidCy;
    const qreal floatCy = filled ? top + kFilledLabelCy : top;
    const qreal lx = filled ? pl : lerp(pl, kPad, fl);
    textLayer(kSlotLabel, Kit::styledFont(m_font, px, m_fontWeight), m_label,
              labelCol, lx, lerp(restCy, floatCy, fl));
  }

  if (m_text.isEmpty() && !composing() && !m_placeholder.isEmpty() &&
      (!hasLabel || fl > 0.5)) {
    const QFont tf = textFont();
    const QString ph = QFontMetricsF(tf).elidedText(
        m_placeholder, Qt::ElideRight, viewWidth());
    textLayer(kSlotPlaceholder, tf, ph, variant, pl, textCenter());
  }

  const qreal iconCy = top + kMidCy;
  if (hasLeading())
    iconLayer(m, kSlotLead, m_leadCache, m_leadPath, m_leadSource, m_leadText,
              variant, QPointF(kIconCx, iconCy));
  if (hasTrailing()) {
    const QColor tc = m_error ? err : variant;
    const QPointF c(w - kIconCx, iconCy);
    const qreal layer = Kit::stateLayer(m_trailPress);
    if (layer > 0.0)
      m.fillCircle(c, kTrailLayerR, Kit::withAlpha(tc, layer));
    iconLayer(m, kSlotTrail, m_trailCache, m_trailPath, m_trailSource,
              m_trailText, tc, c);
  }

  if (!m_support.isEmpty())
    textLayer(kSlotSupport, floatFont, m_support, m_error ? err : variant, kPad,
              top + kFieldH + 4.0 + kSmallPx * 0.75);
}
