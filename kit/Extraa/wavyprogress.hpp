#pragma once
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QQuickWindow>
#include <QTimer>

class WavyProgress : public SgItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged)
  Q_PROPERTY(
      qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
  Q_PROPERTY(bool indeterminate READ indeterminate WRITE setIndeterminate NOTIFY
                 indeterminateChanged)
  Q_PROPERTY(
      bool animated READ animated WRITE setAnimated NOTIFY animatedChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor trackColor READ trackColor WRITE setTrackColor NOTIFY
                 trackColorChanged)
  Q_PROPERTY(
      qreal amplitude READ amplitude WRITE setAmplitude NOTIFY amplitudeChanged)
  Q_PROPERTY(qreal wavelength READ wavelength WRITE setWavelength NOTIFY
                 wavelengthChanged)
  Q_PROPERTY(
      qreal waveSpeed READ waveSpeed WRITE setWaveSpeed NOTIFY waveSpeedChanged)
  Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY
                 strokeWidthChanged)
  Q_PROPERTY(qreal gap READ gap WRITE setGap NOTIFY gapChanged)

public:
  enum Style { Linear, Circular };
  Q_ENUM(Style)

  explicit WavyProgress(QQuickItem *parent = nullptr);

  Style style() const { return m_style; }
  qreal progress() const { return m_progress; }
  bool indeterminate() const { return m_indeterminate; }
  bool animated() const { return m_animated; }
  QColor color() const { return m_color; }
  QColor trackColor() const { return m_trackColor; }
  qreal amplitude() const { return m_amp; }
  qreal wavelength() const { return m_wavelength; }
  qreal waveSpeed() const { return m_waveSpeed; }
  qreal strokeWidth() const { return m_stroke; }
  qreal gap() const { return m_gap; }

  void setStyle(Style v);
  void setProgress(qreal v);
  void setIndeterminate(bool v);
  void setAnimated(bool v);
  void setColor(const QColor &v);
  void setTrackColor(const QColor &v);
  void setAmplitude(qreal v);
  void setWavelength(qreal v);
  void setWaveSpeed(qreal v);
  void setStrokeWidth(qreal v);
  void setGap(qreal v);

signals:
  void styleChanged();
  void progressChanged();
  void indeterminateChanged();
  void animatedChanged();
  void colorChanged();
  void trackColorChanged();
  void amplitudeChanged();
  void wavelengthChanged();
  void waveSpeedChanged();
  void strokeWidthChanged();
  void gapChanged();

protected:
  void build(Kit::Mesh &m) override;
  void componentComplete() override;
  void itemChange(ItemChange change, const ItemChangeData &data) override;
  void geometryChange(const QRectF &n, const QRectF &o) override;
  void propertyAssigned() override;

private:
  qreal targetAmp() const { return m_animated ? m_amp : 0.0; }
  bool needsFrames() const;
  void syncFrames(QQuickWindow *w);
  void onFrame();
  void segment(qreal &a, qreal &b) const;
  void buildLinear(Kit::Mesh &m, qreal a, qreal b);
  void buildCircular(Kit::Mesh &m, qreal a, qreal b);

  Style m_style = Linear;
  qreal m_progress = 0.0;
  bool m_indeterminate = true;
  bool m_animated = true;
  QColor m_color = QColor("#6750A4");
  QColor m_trackColor = QColor("#E8DEF8");
  qreal m_amp = 3.0;
  qreal m_ampNow = 0.0;
  qreal m_wavelength = 28.0;
  qreal m_waveSpeed = 0.8;
  qreal m_stroke = 4.0;
  qreal m_gap = 4.0;
  qreal m_phase = 0.0;
  qreal m_time = 0.0;
  QElapsedTimer m_clock;
  QTimer m_timer;
};
