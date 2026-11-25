#pragma once

#include "../Core/paintutil.hpp"
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QQuickItem>
#include <QRectF>

class MaterialTimePicker : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(int hours READ hours WRITE setHours NOTIFY hoursChanged)
  Q_PROPERTY(int minutes READ minutes WRITE setMinutes NOTIFY minutesChanged)
  Q_PROPERTY(int seconds READ seconds WRITE setSeconds NOTIFY secondsChanged)
  Q_PROPERTY(int totalSeconds READ totalSeconds WRITE setTotalSeconds NOTIFY
                 totalSecondsChanged)
  Q_PROPERTY(bool pm READ pm WRITE setPm NOTIFY hoursChanged)
  Q_PROPERTY(bool clock READ clock WRITE setClock NOTIFY clockChanged)
  Q_PROPERTY(bool showSeconds READ showSeconds WRITE setShowSeconds NOTIFY
                 showSecondsChanged)
  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(int minuteStep READ minuteStep WRITE setMinuteStep NOTIFY
                 minuteStepChanged)
  Q_PROPERTY(int secondStep READ secondStep WRITE setSecondStep NOTIFY
                 secondStepChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY styleChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY styleChanged)
  Q_PROPERTY(
      QColor fieldColor READ fieldColor WRITE setFieldColor NOTIFY styleChanged)
  Q_PROPERTY(QColor selectedFieldColor READ selectedFieldColor WRITE
                 setSelectedFieldColor NOTIFY styleChanged)
  Q_PROPERTY(QColor fieldTextColor READ fieldTextColor WRITE setFieldTextColor
                 NOTIFY styleChanged)
  Q_PROPERTY(QColor selectedFieldTextColor READ selectedFieldTextColor WRITE
                 setSelectedFieldTextColor NOTIFY styleChanged)
  Q_PROPERTY(QColor handleColor READ handleColor WRITE setHandleColor NOTIFY
                 styleChanged)
  Q_PROPERTY(QColor handleTextColor READ handleTextColor WRITE
                 setHandleTextColor NOTIFY styleChanged)
  Q_PROPERTY(
      QColor textColor READ textColor WRITE setTextColor NOTIFY styleChanged)
  Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY
                 styleChanged)

public:
  enum Mode { Hours, Minutes, Seconds };
  Q_ENUM(Mode)

  explicit MaterialTimePicker(QQuickItem *parent = nullptr);

  int hours() const { return m_hours; }
  int minutes() const { return m_minutes; }
  int seconds() const { return m_seconds; }
  int totalSeconds() const {
    return m_hours * 3600 + m_minutes * 60 + m_seconds;
  }
  bool pm() const { return m_hours >= 12; }
  bool clock() const { return m_clock; }
  bool showSeconds() const { return m_showSeconds; }
  Mode mode() const { return m_mode; }
  int minuteStep() const { return m_minuteStep; }
  int secondStep() const { return m_secondStep; }
  QFont font() const { return m_font; }
  QColor color() const { return m_color; }
  QColor fieldColor() const { return m_fieldColor; }
  QColor selectedFieldColor() const { return m_selectedFieldColor; }
  QColor fieldTextColor() const { return m_fieldTextColor; }
  QColor selectedFieldTextColor() const { return m_selectedFieldTextColor; }
  QColor handleColor() const { return m_handleColor; }
  QColor handleTextColor() const { return m_handleTextColor; }
  QColor textColor() const { return m_textColor; }
  QColor outlineColor() const { return m_outlineColor; }

  void setHours(int v);
  void setMinutes(int v);
  void setSeconds(int v);
  void setTotalSeconds(int v);
  void setPm(bool v);
  void setClock(bool v);
  void setShowSeconds(bool v);
  void setMode(Mode v);
  void setMinuteStep(int v);
  void setSecondStep(int v);
  void setFont(const QFont &v);
  void setColor(const QColor &v);
  void setFieldColor(const QColor &v);
  void setSelectedFieldColor(const QColor &v);
  void setFieldTextColor(const QColor &v);
  void setSelectedFieldTextColor(const QColor &v);
  void setHandleColor(const QColor &v);
  void setHandleTextColor(const QColor &v);
  void setTextColor(const QColor &v);
  void setOutlineColor(const QColor &v);

signals:
  void hoursChanged();
  void minutesChanged();
  void secondsChanged();
  void totalSecondsChanged();
  void clockChanged();
  void showSecondsChanged();
  void modeChanged();
  void minuteStepChanged();
  void secondStepChanged();
  void styleChanged();
  void moved();

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void propertyAssigned() override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private slots:
  void commitEdit();

private:
  int fieldCount() const { return m_showSeconds ? 3 : 2; }
  qreal rowWidth() const;
  QRectF fieldRect(int i) const;
  QRectF periodRect() const;
  QPointF dialCenter() const;
  QPointF ringPoint(qreal degrees, qreal radius) const;
  int currentValue() const;
  qreal currentDegrees() const;
  void syncAngle(bool immediate = false);
  void updateImplicit();
  void setPressed(bool v);
  void pick(const QPointF &p);
  void advanceMode();
  void startEdit(int i);

  int m_hours = 0;
  int m_minutes = 0;
  int m_seconds = 0;
  bool m_clock = false;
  bool m_showSeconds = true;
  Mode m_mode = Minutes;
  int m_minuteStep = 1;
  int m_secondStep = 5;
  bool m_drag = false;
  QQuickItem *m_input = nullptr;
  int m_editField = -1;
  Spring m_angle;
  Spring m_press;
  QFont m_font;
  QColor m_color = QColor("#E6E0E9");
  QColor m_fieldColor = QColor("#E6E0E9");
  QColor m_selectedFieldColor = QColor("#EADDFF");
  QColor m_fieldTextColor = QColor("#1D1B20");
  QColor m_selectedFieldTextColor = QColor("#21005D");
  QColor m_handleColor = QColor("#6750A4");
  QColor m_handleTextColor = QColor("#FFFFFF");
  QColor m_textColor = QColor("#1D1B20");
  QColor m_outlineColor = QColor("#79747E");
};
