#include "./wavyprogress.hpp"
#include <QGuiApplication>
#include <QList>
#include <QPointF>
#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kTwoPi = 2.0 * M_PI;
constexpr qreal kCycle = 1.8;
constexpr qreal kSpin = 1.4;
constexpr qreal kMaxDt = 0.1;
constexpr int kFrameMs = 40;
constexpr qreal kAmpRate = 9.0;
constexpr qreal kAmpEps = 0.005;
constexpr qreal kSamplesPerWave = 12.0;
constexpr qreal kArcSamplesPerWave = 20.0;
constexpr qreal kTrackStepPx = 2.0;

qreal easeStep(qreal x) {
  x = std::clamp(x, 0.0, 1.0);
  return x * x * (3.0 - 2.0 * x);
}

} // namespace

WavyProgress::WavyProgress(QQuickItem *parent) : SgItem(parent) {
  m_timer.setInterval(kFrameMs);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &WavyProgress::onFrame);
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
          [this] { syncFrames(window()); });
}

void WavyProgress::propertyAssigned() { syncFrames(window()); }

void WavyProgress::componentComplete() {
  SgItem::componentComplete();
  syncFrames(window());
}

void WavyProgress::itemChange(ItemChange change, const ItemChangeData &data) {
  SgItem::itemChange(change, data);
  if (change == ItemSceneChange)
    syncFrames(data.window);
  else if (change == ItemVisibleHasChanged)
    syncFrames(window());
}

void WavyProgress::geometryChange(const QRectF &n, const QRectF &o) {
  SgItem::geometryChange(n, o);
  if (n.size() != o.size())
    update();
}

bool WavyProgress::needsFrames() const {
  if (!isComponentComplete() || !isVisible())
    return false;
  if (QGuiApplication::applicationState() != Qt::ApplicationActive)
    return false;
  if (m_indeterminate)
    return true;
  if (std::abs(m_ampNow - targetAmp()) > kAmpEps)
    return true;
  return m_waveSpeed != 0.0 && m_ampNow > kAmpEps;
}

void WavyProgress::syncFrames(QQuickWindow *w) {
  if (w && needsFrames()) {
    if (!m_timer.isActive()) {
      m_clock.start();
      m_timer.start();
      update();
    }
  } else if (m_timer.isActive()) {
    m_timer.stop();
    if (std::abs(m_ampNow - targetAmp()) <= kAmpEps &&
        m_ampNow != targetAmp()) {
      m_ampNow = targetAmp();
      update();
    }
  }
}

void WavyProgress::onFrame() {
  const qreal dt = std::min(qreal(m_clock.restart()) / 1000.0, kMaxDt);
  m_phase = std::fmod(m_phase + dt * m_waveSpeed * kTwoPi, kTwoPi);
  m_time = std::fmod(m_time + dt, kCycle * kSpin * 100.0);
  const qreal tgt = targetAmp();
  const qreal k = 1.0 - std::exp(-kAmpRate * dt);
  m_ampNow += (tgt - m_ampNow) * k;
  if (std::abs(m_ampNow - tgt) <= kAmpEps)
    m_ampNow = tgt;
  update();
  syncFrames(window());
}

void WavyProgress::setStyle(Style v) {
  assign(m_style, v, &WavyProgress::styleChanged);
}

void WavyProgress::setProgress(qreal v) {
  assign(m_progress, std::clamp(v, 0.0, 1.0), &WavyProgress::progressChanged);
}

void WavyProgress::setIndeterminate(bool v) {
  assign(m_indeterminate, v, &WavyProgress::indeterminateChanged);
}

void WavyProgress::setAnimated(bool v) {
  assign(m_animated, v, &WavyProgress::animatedChanged);
}

void WavyProgress::setColor(const QColor &v) {
  assign(m_color, v, &WavyProgress::colorChanged);
}

void WavyProgress::setTrackColor(const QColor &v) {
  assign(m_trackColor, v, &WavyProgress::trackColorChanged);
}

void WavyProgress::setAmplitude(qreal v) {
  assign(m_amp, std::max(0.0, v), &WavyProgress::amplitudeChanged);
}

void WavyProgress::setWavelength(qreal v) {
  assign(m_wavelength, std::max(4.0, v), &WavyProgress::wavelengthChanged);
}

void WavyProgress::setWaveSpeed(qreal v) {
  assign(m_waveSpeed, v, &WavyProgress::waveSpeedChanged);
}

void WavyProgress::setStrokeWidth(qreal v) {
  assign(m_stroke, std::max(0.5, v), &WavyProgress::strokeWidthChanged);
}

void WavyProgress::setGap(qreal v) {
  assign(m_gap, std::max(0.0, v), &WavyProgress::gapChanged);
}

void WavyProgress::segment(qreal &a, qreal &b) const {
  if (!m_indeterminate) {
    a = 0.0;
    b = m_progress;
    return;
  }
  const qreal u = std::fmod(m_time, kCycle) / kCycle;
  b = easeStep(u / 0.7);
  a = easeStep((u - 0.3) / 0.7);
}

void WavyProgress::build(Kit::Mesh &m) {
  qreal a, b;
  segment(a, b);
  if (m_style == Linear)
    buildLinear(m, a, b);
  else
    buildCircular(m, a, b);
}

void WavyProgress::buildLinear(Kit::Mesh &m, qreal a, qreal b) {
  const qreal pad = m_stroke / 2.0;
  const qreal x0 = pad, x1 = width() - pad, len = x1 - x0;
  if (len <= 0.0)
    return;
  const qreal cy = height() / 2.0;
  const qreal amp =
      std::min(m_ampNow, std::max(0.0, (height() - m_stroke) / 2.0));
  const qreal xa = x0 + len * a, xb = x0 + len * b;
  const qreal g = m_gap + m_stroke;
  if (xa - g > x0)
    m.line(QPointF(x0, cy), QPointF(xa - g, cy), m_stroke, m_trackColor);
  if (xb + g < x1)
    m.line(QPointF(xb + g, cy), QPointF(x1, cy), m_stroke, m_trackColor);
  if (xb - xa < 0.01) {
    if (b > 0.0 || m_indeterminate)
      m.fillCircle(QPointF(xa, cy), m_stroke / 2.0, m_color);
    return;
  }
  if (amp < 0.01) {
    m.line(QPointF(xa, cy), QPointF(xb, cy), m_stroke, m_color);
    return;
  }
  const qreal k = kTwoPi / m_wavelength;
  const qreal ph = m_phase;
  const int segs =
      std::max(2, int(std::ceil((xb - xa) / (m_wavelength / kSamplesPerWave))));
  QList<QPointF> pts;
  pts.reserve(segs + 1);
  for (int i = 0; i <= segs; ++i) {
    const qreal x = xa + (xb - xa) * i / segs;
    pts.append(QPointF(x, cy + amp * std::sin(k * x - ph)));
  }
  m.polyline(pts, m_stroke, m_color);
}

void WavyProgress::buildCircular(Kit::Mesh &m, qreal a, qreal b) {
  const qreal amp = m_ampNow;
  const qreal r = std::min(width(), height()) / 2.0 - m_stroke / 2.0 - amp;
  if (r <= 0.0)
    return;
  const QPointF c(width() / 2.0, height() / 2.0);
  const qreal rot = m_indeterminate ? kTwoPi * (m_time / kSpin) : 0.0;
  auto pt = [&](qreal th, qreal rad) {
    const qreal t = th + rot;
    return QPointF(c.x() + rad * std::sin(t), c.y() - rad * std::cos(t));
  };
  const int n = std::max(1, int(std::round(kTwoPi * r / m_wavelength)));
  const qreal ga = (m_gap + m_stroke) / r;
  const qreal ta = kTwoPi * a, tb = kTwoPi * b;
  const qreal t0 = tb + ga, t1 = kTwoPi + ta - ga;
  if (t1 > t0) {
    const int steps = std::max(2, int(std::ceil((t1 - t0) * r / kTrackStepPx)));
    QList<QPointF> track;
    track.reserve(steps + 1);
    for (int i = 0; i <= steps; ++i)
      track.append(pt(t0 + (t1 - t0) * i / steps, r));
    m.polyline(track, m_stroke, m_trackColor);
  }
  if (tb - ta < 0.001) {
    m.fillCircle(pt(ta, r), m_stroke / 2.0, m_color);
    return;
  }
  const int steps =
      std::max(32, int((tb - ta) / kTwoPi * n * kArcSamplesPerWave));
  QList<QPointF> arc;
  arc.reserve(steps + 1);
  for (int i = 0; i <= steps; ++i) {
    const qreal th = ta + (tb - ta) * i / steps;
    arc.append(pt(th, r + amp * std::sin(n * th - m_phase)));
  }
  m.polyline(arc, m_stroke, m_color);
}
