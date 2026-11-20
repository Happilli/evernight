#include "./materialsearchbox.hpp"
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
constexpr qreal kHeight = 56.0;
constexpr qreal kPad = 16.0;
constexpr qreal kIcon = 24.0;
constexpr qreal kClearIcon = 20.0;
constexpr qreal kGap = 16.0;
constexpr qreal kFontPx = 16.0;
constexpr qreal kHit = 40.0;
constexpr qreal kDefaultW = 360.0;
constexpr qreal kClearHalf = 5.0;
constexpr int kSlotLeading = 0;
constexpr int kSlotTrailing = 1;
constexpr int kSlotClear = 2;
constexpr int kSlotText = 3;

constexpr const char *kInputQml = R"QML(
import QtQuick
TextInput {
  id: ti
  property color accent: "#6750A4"
  clip: true
  autoScroll: true
  inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
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
      running: ti.activeFocus
      onRunningChanged: cur.on = true
      onTriggered: cur.on = !cur.on
    }
  }
}
)QML";
} // namespace

MaterialSearchBox::MaterialSearchBox(QQuickItem *parent)
    : SgMaterialItem(parent, 520.0, 0.6) {
  setAcceptedMouseButtons(Qt::LeftButton);
  setFlag(ItemIsFocusScope, true);
  setImplicitWidth(kDefaultW);
  setImplicitHeight(kHeight);
  retarget(true);
}

MaterialSearchBox::~MaterialSearchBox() { detachFilter(); }

void MaterialSearchBox::componentComplete() {
  SgMaterialItem::componentComplete();
  createInput();
  retarget(true);
  update();
}

void MaterialSearchBox::createInput() {
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
    qWarning("MaterialSearchBox: input creation failed: %s",
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
  connect(m_input, SIGNAL(displayTextChanged()), this, SLOT(update()));
  connect(m_input, &QQuickItem::activeFocusChanged, this,
          &MaterialSearchBox::onInputFocus);
  m_input->setProperty("text", m_text);
  applyStyle();
  layoutInput();
}

void MaterialSearchBox::applyStyle() {
  if (!m_input)
    return;
  m_input->setProperty("color", m_textColor);
  m_input->setProperty("selectedTextColor", m_textColor);
  m_input->setProperty("selectionColor", m_selectionColor);
  m_input->setProperty("accent", m_cursorColor);
  m_input->setProperty("readOnly", m_readOnly);
  m_input->setProperty("maximumLength", m_maxLength > 0 ? m_maxLength : 32767);
  m_input->setProperty("font", labelFont());
}

void MaterialSearchBox::layoutInput() {
  if (!m_input)
    return;
  m_input->setX(textLeft());
  m_input->setY(std::round((height() - kHeight) / 2.0));
  m_input->setWidth(std::max(0.0, textRight() - textLeft()));
  m_input->setHeight(kHeight);
}

void MaterialSearchBox::propertyAssigned() {
  applyStyle();
  layoutInput();
}

void MaterialSearchBox::geometryChange(const QRectF &n, const QRectF &o) {
  SgMaterialItem::geometryChange(n, o);
  if (n.size() != o.size())
    layoutInput();
}

void MaterialSearchBox::itemChange(ItemChange change,
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

void MaterialSearchBox::attachFilter() {
  QQuickWindow *w = window();
  if (!w || m_filtered == w)
    return;
  detachFilter();
  w->installEventFilter(this);
  m_filtered = w;
}

void MaterialSearchBox::detachFilter() {
  if (m_filtered)
    m_filtered->removeEventFilter(this);
  m_filtered = nullptr;
}

bool MaterialSearchBox::eventFilter(QObject *watched, QEvent *event) {
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

void MaterialSearchBox::onInputFocus(bool focused) {
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

void MaterialSearchBox::focusInput(Qt::FocusReason reason) {
  if (!m_input)
    return;
  m_input->setFocus(true);
  if (!hasActiveFocus())
    forceActiveFocus(reason);
}

QFont MaterialSearchBox::labelFont() const {
  return Kit::styledFont(m_font, kFontPx, m_fontWeight);
}

bool MaterialSearchBox::leadingShown() const {
  return m_showSearchIcon || !m_leadingIconText.isEmpty() ||
         !m_leadingIconSource.isEmpty();
}

bool MaterialSearchBox::trailingShown() const {
  return !m_trailingIconText.isEmpty() || !m_trailingIconSource.isEmpty();
}

bool MaterialSearchBox::clearVisible() const {
  return m_clearable && !m_readOnly && !m_text.isEmpty();
}

bool MaterialSearchBox::composing() const {
  return m_input && !m_input->property("displayText").toString().isEmpty();
}

qreal MaterialSearchBox::leadingCx() const { return kPad + kIcon / 2.0; }

qreal MaterialSearchBox::trailingCx() const {
  return width() - kPad - kIcon / 2.0;
}

qreal MaterialSearchBox::clearCx() const {
  return trailingShown() ? trailingCx() - kHit : trailingCx();
}

qreal MaterialSearchBox::textLeft() const {
  return leadingShown() ? kPad + kIcon + kGap : kPad;
}

qreal MaterialSearchBox::textRight() const {
  qreal r = width() - kPad;
  if (m_clearable)
    r = clearCx() - kIcon / 2.0 - kGap / 2.0;
  else if (trailingShown())
    r = trailingCx() - kIcon / 2.0 - kGap / 2.0;
  return std::max(textLeft(), r);
}

int MaterialSearchBox::zoneAt(const QPointF &pt) const {
  const qreal cy = height() / 2.0;
  const qreal half = kHit / 2.0;
  if (std::abs(pt.y() - cy) <= half) {
    if (leadingShown() && std::abs(pt.x() - leadingCx()) <= half)
      return Leading;
    if (trailingShown() && std::abs(pt.x() - trailingCx()) <= half)
      return Trailing;
    if (clearVisible() && std::abs(pt.x() - clearCx()) <= half)
      return Clear;
  }
  const QRectF bar(0.0, std::round((height() - kHeight) / 2.0), width(),
                   kHeight);
  return bar.contains(pt) ? Text : None;
}

int MaterialSearchBox::cursorPosition() const {
  return m_input ? m_input->property("cursorPosition").toInt() : 0;
}

void MaterialSearchBox::onInputText() {
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

void MaterialSearchBox::onInputEdited() { emit textEdited(); }
void MaterialSearchBox::onInputAccepted() { emit accepted(); }
void MaterialSearchBox::onInputCursor() { emit cursorPositionChanged(); }

void MaterialSearchBox::retarget(bool immediate) {
  m_foc.go(m_focused ? 1.0 : 0.0, immediate);
  m_clr.go(clearVisible() ? 1.0 : 0.0, immediate);
  if (!immediate)
    animate();
}

bool MaterialSearchBox::advance(qreal dt) {
  bool active = step(m_foc, dt);
  active |= step(m_clr, dt, 0.001, 1.2);
  return active;
}

void MaterialSearchBox::setText(const QString &v) {
  QString t = v;
  if (m_maxLength > 0)
    t = t.left(m_maxLength);
  if (m_input) {
    m_input->setProperty("text", t);
    return;
  }
  if (t == m_text)
    return;
  m_text = t;
  emit textChanged();
  retarget();
  update();
}

void MaterialSearchBox::setPlaceholderText(const QString &v) {
  assign(m_placeholder, v, &MaterialSearchBox::placeholderTextChanged);
}

void MaterialSearchBox::setFont(const QFont &v) {
  assign(m_font, v, &MaterialSearchBox::fontChanged);
}

void MaterialSearchBox::setFontWeight(int v) {
  v = qBound(1, v, 1000);
  assign(m_fontWeight, v, &MaterialSearchBox::fontWeightChanged);
}

void MaterialSearchBox::setIconFont(const QFont &v) {
  assign(m_iconFont, v, &MaterialSearchBox::iconFontChanged);
}

void MaterialSearchBox::setLeadingIconText(const QString &v) {
  assign(m_leadingIconText, v, &MaterialSearchBox::leadingIconTextChanged);
}

void MaterialSearchBox::setTrailingIconText(const QString &v) {
  assign(m_trailingIconText, v, &MaterialSearchBox::trailingIconTextChanged);
}

void MaterialSearchBox::setLeadingIconSource(const QUrl &v) {
  if (assign(m_leadingIconSource, v,
             &MaterialSearchBox::leadingIconSourceChanged))
    m_leading.path.clear();
}

void MaterialSearchBox::setTrailingIconSource(const QUrl &v) {
  if (assign(m_trailingIconSource, v,
             &MaterialSearchBox::trailingIconSourceChanged))
    m_trailing.path.clear();
}

void MaterialSearchBox::setClearIconSource(const QUrl &v) {
  if (assign(m_clearIconSource, v, &MaterialSearchBox::clearIconSourceChanged))
    m_clearSlot.path.clear();
}

void MaterialSearchBox::setShowSearchIcon(bool v) {
  assign(m_showSearchIcon, v, &MaterialSearchBox::showSearchIconChanged);
}

void MaterialSearchBox::setClearable(bool v) {
  if (assign(m_clearable, v, &MaterialSearchBox::clearableChanged))
    retarget();
}

void MaterialSearchBox::setReadOnly(bool v) {
  if (assign(m_readOnly, v, &MaterialSearchBox::readOnlyChanged))
    retarget();
}

void MaterialSearchBox::setMaxLength(int v) {
  v = std::max(0, v);
  if (assign(m_maxLength, v, &MaterialSearchBox::maxLengthChanged) && v > 0 &&
      m_text.size() > v)
    setText(m_text.left(v));
}

void MaterialSearchBox::setColor(const QColor &v) {
  assign(m_color, v, &MaterialSearchBox::colorChanged);
}

void MaterialSearchBox::setTextColor(const QColor &v) {
  assign(m_textColor, v, &MaterialSearchBox::textColorChanged);
}

void MaterialSearchBox::setPlaceholderColor(const QColor &v) {
  assign(m_placeholderColor, v, &MaterialSearchBox::placeholderColorChanged);
}

void MaterialSearchBox::setIconColor(const QColor &v) {
  assign(m_iconColor, v, &MaterialSearchBox::iconColorChanged);
}

void MaterialSearchBox::setBorderColor(const QColor &v) {
  assign(m_borderColor, v, &MaterialSearchBox::borderColorChanged);
}

void MaterialSearchBox::setBorderWidth(qreal v) {
  assign(m_borderWidth, std::max(0.0, v),
         &MaterialSearchBox::borderWidthChanged);
}

void MaterialSearchBox::setCursorColor(const QColor &v) {
  assign(m_cursorColor, v, &MaterialSearchBox::cursorColorChanged);
}

void MaterialSearchBox::setSelectionColor(const QColor &v) {
  assign(m_selectionColor, v, &MaterialSearchBox::selectionColorChanged);
}

void MaterialSearchBox::clear() {
  if (m_text.isEmpty())
    return;
  if (m_input)
    m_input->setProperty("text", QString());
  else
    setText(QString());
  emit textEdited();
  emit cleared();
}

void MaterialSearchBox::selectAll() {
  if (m_input)
    QMetaObject::invokeMethod(m_input, "selectAll");
}

void MaterialSearchBox::blur() {
  if (m_input)
    m_input->setFocus(false);
  setFocus(false);
  QGuiApplication::inputMethod()->hide();
}

void MaterialSearchBox::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled()) {
    e->ignore();
    return;
  }
  const int z = zoneAt(e->position());
  if (z == None) {
    e->ignore();
    return;
  }
  focusInput(Qt::MouseFocusReason);
  m_pressZone = z;
  update();
  e->accept();
}

void MaterialSearchBox::mouseReleaseEvent(QMouseEvent *e) {
  if (m_pressZone == None)
    return;
  const int pressed = m_pressZone;
  m_pressZone = None;
  const bool inside = zoneAt(e->position()) == pressed;
  update();
  if (inside) {
    if (pressed == Leading)
      emit leadingClicked();
    else if (pressed == Trailing)
      emit trailingClicked();
    else if (pressed == Clear)
      clear();
    else if (pressed == Text && !m_readOnly)
      QGuiApplication::inputMethod()->show();
  }
  e->accept();
}

void MaterialSearchBox::mouseUngrabEvent() {
  m_pressZone = None;
  update();
}

void MaterialSearchBox::iconLayer(Kit::Mesh &m, int slot, IconSlot &s,
                                  const QUrl &src, qreal cx, qreal cy,
                                  qreal size) {
  const qreal dpr = m.dpr();
  const QColor col = Kit::withAlpha(m_iconColor, m_iconColor.alphaF() * fade());
  if (s.path.isEmpty())
    s.path = Kit::resolveUrl(qmlContext(this), src);
  const QString key =
      QStringList{"i", src.toString(), col.name(QColor::HexArgb),
                  QString::number(size), QString::number(dpr)}
          .join(QLatin1Char('|'));
  useLayer(slot, key,
           QRectF(m.snap(cx - size / 2.0), m.snap(cy - size / 2.0), size, size),
           [&] { return s.cache.get(s.path, size, dpr, col); });
}

void MaterialSearchBox::build(Kit::Mesh &m) {
  const qreal w = width(), h = height();
  if (w <= 0.0 || h <= 0.0)
    return;

  const QRectF bar(0.0, std::round((h - kHeight) / 2.0), w, kHeight);
  const qreal cy = h / 2.0;
  const qreal full = std::min(bar.width(), bar.height()) / 2.0;

  m.fillRoundedRect(bar, full, full, full, full, m_color);

  const qreal foc = std::clamp(m_foc.x, 0.0, 1.0);
  if (m_borderWidth > 0.0) {
    const QColor bc = Kit::mix(m_borderColor, m_cursorColor, foc);
    if (bc.alpha() > 0)
      m.strokeRoundedRect(bar, full, full, full, full,
                          m.snapStroke(m_borderWidth), bc);
  }

  auto zoneLayer = [&](int zone, qreal cx, qreal scale) {
    const qreal a = Kit::stateLayer(m_pressZone == zone);
    if (a <= 0.0)
      return;
    m.fillCircle(QPointF(cx, cy), kHit / 2.0 * scale,
                 Kit::withAlpha(m_iconColor, a));
  };

  if (leadingShown()) {
    const qreal cx = leadingCx();
    zoneLayer(Leading, cx, 1.0);
    if (!m_leadingIconSource.isEmpty()) {
      iconLayer(m, kSlotLeading, m_leading, m_leadingIconSource, cx, cy, kIcon);
    } else if (!m_leadingIconText.isEmpty()) {
      glyphLayer(kSlotLeading, m_iconFont, m_leadingIconText, m_iconColor,
                 QPointF(cx, cy), kIcon);
    } else {
      m.strokeCircle(QPointF(cx - 2.0, cy - 2.0), 7.5, m.snapStroke(2.0),
                     m_iconColor);
      m.line(QPointF(cx + 2.7, cy + 2.7), QPointF(cx + 8.0, cy + 8.0),
             m.snapStroke(2.0), m_iconColor);
    }
  }

  if (trailingShown()) {
    const qreal cx = trailingCx();
    zoneLayer(Trailing, cx, 1.0);
    if (!m_trailingIconSource.isEmpty())
      iconLayer(m, kSlotTrailing, m_trailing, m_trailingIconSource, cx, cy,
                kIcon);
    else
      glyphLayer(kSlotTrailing, m_iconFont, m_trailingIconText, m_iconColor,
                 QPointF(cx, cy), kIcon);
  }

  const qreal csRaw = std::clamp(m_clr.x, 0.0, 1.4);
  const qreal cs = std::min(csRaw, 1.0);
  if (csRaw > 0.01) {
    const qreal cx = clearCx();
    zoneLayer(Clear, cx, csRaw);
    if (!m_clearIconSource.isEmpty()) {
      if (cs > 0.5)
        iconLayer(m, kSlotClear, m_clearSlot, m_clearIconSource, cx, cy,
                  kClearIcon);
    } else {
      const qreal half = kClearHalf * csRaw;
      const QColor col = Kit::withAlpha(m_iconColor, m_iconColor.alphaF() * cs);
      const qreal lw = m.snapStroke(2.0 * csRaw);
      m.line(QPointF(cx - half, cy - half), QPointF(cx + half, cy + half), lw,
             col);
      m.line(QPointF(cx - half, cy + half), QPointF(cx + half, cy - half), lw,
             col);
    }
  }

  if (m_text.isEmpty() && !composing() && !m_placeholder.isEmpty()) {
    const qreal avail = std::max(0.0, textRight() - textLeft());
    if (avail >= 1.0) {
      const QFont lf = labelFont();
      const QString ph =
          QFontMetricsF(lf).elidedText(m_placeholder, Qt::ElideRight, avail);
      textLayer(kSlotText, lf, ph, m_placeholderColor, textLeft(), cy);
    }
  }
}
