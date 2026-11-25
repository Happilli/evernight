#include "./materialtimepicker.hpp"
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QInputMethod>
#include <QMetaObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kDial = 256.0;
constexpr qreal kFieldW = 60.0;
constexpr qreal kFieldH = 76.0;
constexpr qreal kColonW = 10.0;
constexpr qreal kPeriodGap = 8.0;
constexpr qreal kPeriodW = 44.0;
constexpr qreal kGap = 32.0;
constexpr qreal kRing = 101.0;
constexpr qreal kHandleR = 24.0;
constexpr qreal kCenterR = 4.0;
constexpr qreal kLineW = 2.0;
constexpr qreal kFieldRadius = 8.0;
constexpr qreal kFieldFont = 38.0;
constexpr qreal kPeriodFont = 16.0;
constexpr qreal kLabelFont = 16.0;
constexpr qreal kPressGrow = 0.18;
constexpr qreal kPressStiffness = 1.4;
constexpr qreal kAngleEps = 0.05;
constexpr int kSlotHours = 12;
constexpr int kSlotMinutes = 13;
constexpr int kSlotSeconds = 14;
constexpr int kSlotColon1 = 15;
constexpr int kSlotColon2 = 16;
constexpr int kSlotAm = 17;
constexpr int kSlotPm = 18;
constexpr const char *kInputQml = R"QML(
import QtQuick
TextInput {
    visible: false
    clip: true
    maximumLength: 2
    inputMethodHints: Qt.ImhDigitsOnly
    horizontalAlignment: TextInput.AlignHCenter
    verticalAlignment: TextInput.AlignVCenter
}
)QML";
} // namespace

MaterialTimePicker::MaterialTimePicker(QQuickItem *parent)
    : SgMaterialItem(parent, 420.0, 0.7) {
  setAcceptedMouseButtons(Qt::LeftButton);
  updateImplicit();
  m_angle.go(currentDegrees(), true);
}

void MaterialTimePicker::setHours(int v) {
  v = qBound(0, v, m_clock ? 23 : 12);
  if (assign(m_hours, v, &MaterialTimePicker::hoursChanged))
    emit totalSecondsChanged();
}

void MaterialTimePicker::setMinutes(int v) {
  if (assign(m_minutes, qBound(0, v, 59), &MaterialTimePicker::minutesChanged))
    emit totalSecondsChanged();
}

void MaterialTimePicker::setSeconds(int v) {
  if (assign(m_seconds, qBound(0, v, 59), &MaterialTimePicker::secondsChanged))
    emit totalSecondsChanged();
}

void MaterialTimePicker::setTotalSeconds(int v) {
  const int maxH = m_clock ? 23 : 12;
  v = qBound(0, v, maxH * 3600 + 3599);
  setHours(v / 3600);
  setMinutes((v % 3600) / 60);
  setSeconds(v % 60);
}

void MaterialTimePicker::setPm(bool v) {
  if (!m_clock || pm() == v)
    return;
  setHours(m_hours % 12 + (v ? 12 : 0));
}

void MaterialTimePicker::setClock(bool v) {
  if (!assign(m_clock, v, &MaterialTimePicker::clockChanged))
    return;
  if (!m_clock && m_hours > 11)
    setHours(m_hours % 12);
}

void MaterialTimePicker::setShowSeconds(bool v) {
  if (!assign(m_showSeconds, v, &MaterialTimePicker::showSecondsChanged))
    return;
  if (!m_showSeconds && m_mode == Seconds)
    setMode(Minutes);
}

void MaterialTimePicker::setMode(Mode v) {
  if (v == Seconds && !m_showSeconds)
    v = Minutes;
  assign(m_mode, v, &MaterialTimePicker::modeChanged);
}

void MaterialTimePicker::setMinuteStep(int v) {
  assign(m_minuteStep, std::max(1, v), &MaterialTimePicker::minuteStepChanged);
}

void MaterialTimePicker::setSecondStep(int v) {
  assign(m_secondStep, std::max(1, v), &MaterialTimePicker::secondStepChanged);
}

void MaterialTimePicker::setFont(const QFont &v) {
  assign(m_font, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setColor(const QColor &v) {
  assign(m_color, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setFieldColor(const QColor &v) {
  assign(m_fieldColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setSelectedFieldColor(const QColor &v) {
  assign(m_selectedFieldColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setFieldTextColor(const QColor &v) {
  assign(m_fieldTextColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setSelectedFieldTextColor(const QColor &v) {
  assign(m_selectedFieldTextColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setHandleColor(const QColor &v) {
  assign(m_handleColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setHandleTextColor(const QColor &v) {
  assign(m_handleTextColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setTextColor(const QColor &v) {
  assign(m_textColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::setOutlineColor(const QColor &v) {
  assign(m_outlineColor, v, &MaterialTimePicker::styleChanged);
}

void MaterialTimePicker::propertyAssigned() {
  updateImplicit();
  syncAngle();
}

qreal MaterialTimePicker::rowWidth() const {
  const int n = fieldCount();
  qreal w = n * kFieldW + (n - 1) * kColonW;
  if (m_clock)
    w += kPeriodGap + kPeriodW;
  return w;
}

void MaterialTimePicker::updateImplicit() {
  setImplicitWidth(std::max(kDial, rowWidth()));
  setImplicitHeight(kFieldH + kGap + kDial);
}

QRectF MaterialTimePicker::fieldRect(int i) const {
  const qreal x0 = (width() - rowWidth()) / 2.0;
  return QRectF(x0 + i * (kFieldW + kColonW), 0.0, kFieldW, kFieldH);
}

QRectF MaterialTimePicker::periodRect() const {
  const QRectF last = fieldRect(fieldCount() - 1);
  return QRectF(last.right() + kPeriodGap, 0.0, kPeriodW, kFieldH);
}

QPointF MaterialTimePicker::dialCenter() const {
  return QPointF(width() / 2.0, kFieldH + kGap + kDial / 2.0);
}

QPointF MaterialTimePicker::ringPoint(qreal degrees, qreal radius) const {
  const qreal r = degrees * M_PI / 180.0;
  return dialCenter() + QPointF(std::sin(r), -std::cos(r)) * radius;
}

int MaterialTimePicker::currentValue() const {
  if (m_mode == Hours)
    return m_hours;
  return m_mode == Minutes ? m_minutes : m_seconds;
}

qreal MaterialTimePicker::currentDegrees() const {
  if (m_mode == Hours)
    return (m_hours % 12) * 30.0;
  return currentValue() * 6.0;
}

void MaterialTimePicker::syncAngle(bool immediate) {
  const qreal target = currentDegrees();
  const qreal cur = m_angle.x;
  qreal delta = std::fmod(target - cur, 360.0);
  if (delta > 180.0)
    delta -= 360.0;
  else if (delta < -180.0)
    delta += 360.0;
  m_angle.go(cur + delta, immediate);
  if (!immediate)
    animate();
}

void MaterialTimePicker::setPressed(bool v) {
  m_press.go(v ? 1.0 : 0.0);
  animate();
  update();
}

bool MaterialTimePicker::advance(qreal dt) {
  const bool a = step(m_angle, dt, kAngleEps);
  const bool b = step(m_press, dt, 0.001, kPressStiffness);
  return a || b;
}

void MaterialTimePicker::advanceMode() {
  if (m_mode == Hours)
    setMode(Minutes);
  else if (m_mode == Minutes && m_showSeconds)
    setMode(Seconds);
}

void MaterialTimePicker::pick(const QPointF &p) {
  const QPointF d = p - dialCenter();
  qreal deg = std::atan2(d.x(), -d.y()) * 180.0 / M_PI;
  if (deg < 0.0)
    deg += 360.0;
  if (m_mode == Hours) {
    const int idx = int(std::lround(deg / 30.0)) % 12;
    int h;
    if (m_clock)
      h = idx + (pm() ? 12 : 0);
    else
      h = idx == 0 ? 12 : idx;
    if (h != m_hours) {
      setHours(h);
      emit moved();
    }
  } else if (m_mode == Minutes) {
    const int v =
        (int(std::lround(deg / 6.0 / m_minuteStep)) * m_minuteStep) % 60;
    if (v != m_minutes) {
      setMinutes(v);
      emit moved();
    }
  } else {
    const int v =
        (int(std::lround(deg / 6.0 / m_secondStep)) * m_secondStep) % 60;
    if (v != m_seconds) {
      setSeconds(v);
      emit moved();
    }
  }
}

void MaterialTimePicker::startEdit(int i) {
  if (!m_input) {
    QQmlEngine *engine = qmlEngine(this);
    if (!engine)
      return;
    QQmlComponent comp(engine);
    comp.setData(kInputQml, QUrl());
    QObject *obj = comp.create(qmlContext(this));
    m_input = qobject_cast<QQuickItem *>(obj);
    if (!m_input) {
      delete obj;
      return;
    }
    m_input->setParent(this);
    m_input->setParentItem(this);
    connect(m_input, SIGNAL(accepted()), this, SLOT(commitEdit()));
    connect(m_input, &QQuickItem::activeFocusChanged, this, [this](bool f) {
      if (!f)
        commitEdit();
    });
  }
  const QRectF r = fieldRect(i);
  int shown = i == 0 ? m_hours : (i == 1 ? m_minutes : m_seconds);
  if (i == 0 && m_clock)
    shown = m_hours % 12 == 0 ? 12 : m_hours % 12;
  m_editField = i;
  m_input->setX(r.x());
  m_input->setY(r.y());
  m_input->setWidth(r.width());
  m_input->setHeight(r.height());
  m_input->setProperty("font",
                       Kit::styledFont(m_font, kFieldFont, QFont::Normal));
  m_input->setProperty("color", m_selectedFieldTextColor);
  m_input->setProperty("selectedTextColor", m_selectedFieldTextColor);
  m_input->setProperty("selectionColor", Kit::withAlpha(m_handleColor, 0.3));
  m_input->setProperty("text", QString::number(shown).rightJustified(2, '0'));
  m_input->setVisible(true);
  QMetaObject::invokeMethod(m_input, "forceActiveFocus");
  QMetaObject::invokeMethod(m_input, "selectAll");
  QGuiApplication::inputMethod()->show();
  update();
}

void MaterialTimePicker::commitEdit() {
  const int i = m_editField;
  if (i < 0 || !m_input)
    return;
  m_editField = -1;
  const int v = m_input->property("text").toString().toInt();
  m_input->setVisible(false);
  QGuiApplication::inputMethod()->hide();
  if (i == 0) {
    if (m_clock)
      setHours(v % 12 + (pm() ? 12 : 0));
    else
      setHours(v);
  } else if (i == 1) {
    setMinutes(v);
  } else {
    setSeconds(v);
  }
  emit moved();
  syncAngle();
  update();
}

void MaterialTimePicker::mousePressEvent(QMouseEvent *e) {
  if (!isEnabled()) {
    e->ignore();
    return;
  }
  if (m_editField >= 0)
    commitEdit();
  const QPointF p = e->position();
  for (int i = 0; i < fieldCount(); ++i) {
    if (fieldRect(i).contains(p)) {
      if (m_mode == Mode(i))
        startEdit(i);
      else
        setMode(Mode(i));
      e->accept();
      return;
    }
  }
  if (m_clock && periodRect().contains(p)) {
    setPm(p.y() > periodRect().center().y());
    emit moved();
    e->accept();
    return;
  }
  const QPointF d = p - dialCenter();
  if (std::hypot(d.x(), d.y()) <= kDial / 2.0) {
    m_drag = true;
    setKeepMouseGrab(true);
    setPressed(true);
    pick(p);
    e->accept();
    return;
  }
  e->ignore();
}

void MaterialTimePicker::mouseMoveEvent(QMouseEvent *e) {
  if (!m_drag)
    return;
  pick(e->position());
  e->accept();
}

void MaterialTimePicker::mouseReleaseEvent(QMouseEvent *e) {
  if (!m_drag)
    return;
  pick(e->position());
  m_drag = false;
  setKeepMouseGrab(false);
  setPressed(false);
  advanceMode();
  e->accept();
}

void MaterialTimePicker::mouseUngrabEvent() {
  m_drag = false;
  setKeepMouseGrab(false);
  setPressed(false);
}

void MaterialTimePicker::build(Kit::Mesh &m) {
  if (width() <= 0.0)
    return;
  const bool hm = m_mode == Hours;
  const QFont big = Kit::styledFont(m_font, kFieldFont, QFont::Normal);
  const QFont small = Kit::styledFont(m_font, kLabelFont, QFont::Normal);
  const QFont periodFont = Kit::styledFont(m_font, kPeriodFont, QFont::Medium);

  auto centered = [&](int slot, const QFont &f, const QString &t,
                      const QColor &c, const QPointF &p) {
    const qreal w = QFontMetricsF(f).horizontalAdvance(t);
    textLayer(slot, f, t, c, p.x() - w / 2.0, p.y());
  };
  auto two = [](int v) { return QString::number(v).rightJustified(2, '0'); };

  int shownHours = m_hours;
  if (m_clock)
    shownHours = m_hours % 12 == 0 ? 12 : m_hours % 12;

  const int values[3] = {shownHours, m_minutes, m_seconds};
  const int slotIds[3] = {kSlotHours, kSlotMinutes, kSlotSeconds};
  for (int i = 0; i < fieldCount(); ++i) {
    const QRectF r = fieldRect(i);
    const bool on = int(m_mode) == i;
    m.fillRoundedRect(r, kFieldRadius, kFieldRadius, kFieldRadius, kFieldRadius,
                      on ? m_selectedFieldColor : m_fieldColor);
    if (m_editField != i)
      centered(slotIds[i], big, two(values[i]),
               on ? m_selectedFieldTextColor : m_fieldTextColor, r.center());
  }
  centered(kSlotColon1, big, QStringLiteral(":"), m_fieldTextColor,
           QPointF(fieldRect(0).right() + kColonW / 2.0, kFieldH / 2.0));
  if (m_showSeconds)
    centered(kSlotColon2, big, QStringLiteral(":"), m_fieldTextColor,
             QPointF(fieldRect(1).right() + kColonW / 2.0, kFieldH / 2.0));

  if (m_clock) {
    const QRectF pr = periodRect();
    const qreal half = kFieldH / 2.0;
    const QRectF top(pr.left(), pr.top(), pr.width(), half);
    const QRectF bottom(pr.left(), pr.top() + half, pr.width(), half);
    const bool isPm = pm();
    m.fillRoundedRect(top, kFieldRadius, kFieldRadius, 0.0, 0.0,
                      isPm ? m_fieldColor : m_selectedFieldColor);
    m.fillRoundedRect(bottom, 0.0, 0.0, kFieldRadius, kFieldRadius,
                      isPm ? m_selectedFieldColor : m_fieldColor);
    m.strokeRoundedRect(pr, kFieldRadius, kFieldRadius, kFieldRadius,
                        kFieldRadius, 1.0, m_outlineColor);
    m.line(QPointF(pr.left(), pr.top() + half),
           QPointF(pr.right(), pr.top() + half), 1.0, m_outlineColor);
    centered(kSlotAm, periodFont, QStringLiteral("AM"),
             isPm ? m_fieldTextColor : m_selectedFieldTextColor, top.center());
    centered(kSlotPm, periodFont, QStringLiteral("PM"),
             isPm ? m_selectedFieldTextColor : m_fieldTextColor,
             bottom.center());
  }

  const QPointF c = dialCenter();
  m.fillCircle(c, kDial / 2.0, m_color);

  const qreal press = std::max(0.0, m_press.x);
  const QPointF hp = ringPoint(m_angle.x, kRing);
  m.line(c, hp, kLineW, m_handleColor);
  m.fillCircle(hp, kHandleR * (1.0 + kPressGrow * press), m_handleColor);
  m.fillCircle(c, kCenterR, m_handleColor);

  const int value = currentValue();
  const int sel = hm ? value % 12 : (value % 5 == 0 ? (value / 5) % 12 : -1);
  if (sel < 0)
    m.fillCircle(hp, kCenterR, m_handleTextColor);

  for (int i = 0; i < 12; ++i) {
    QString label;
    if (hm)
      label = QString::number(i == 0 ? 12 : i);
    else
      label = QString::number(i == 0 ? 60 : i * 5);
    centered(i, small, label, i == sel ? m_handleTextColor : m_textColor,
             ringPoint(i * 30.0, kRing));
  }
}
