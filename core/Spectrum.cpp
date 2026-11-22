#include "./Spectrum.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kAttack[Spectrum::kBands] = {0.018f, 0.028f, 0.040f};
constexpr float kRelease[Spectrum::kBands] = {0.085f, 0.120f, 0.160f};
constexpr float kRange[Spectrum::kBands] = {26.0f, 31.0f, 38.0f};
constexpr float kCurve[Spectrum::kBands] = {1.25f, 1.50f, 1.75f};
constexpr float kPeakFall[Spectrum::kBands] = {0.15f, 0.10f, 0.07f};
constexpr float kDisplayTau = 0.022f;
constexpr qint64 kDriftMs = 150;
} // namespace

Spectrum::Spectrum(QObject *parent)
    : QObject(parent), m_out(this), m_levels(kBands, 0.0) {
  constexpr int bits = 10;
  for (int i = 0; i < kFft; ++i) {
    m_win[i] = 0.5f * (1.0f - std::cos(2.0f * float(M_PI) * i / (kFft - 1)));
    int r = 0;
    for (int b = 0; b < bits; ++b)
      if (i & (1 << b))
        r |= 1 << (bits - 1 - b);
    m_rev[i] = r;
  }
  for (int k = 0; k < kFft / 2; ++k)
    m_tw[k] = std::polar(1.0f, -2.0f * float(M_PI) * k / kFft);
  m_peak.fill(-40.0f);
  m_raw.fill(0.0f);
  m_clock.start();
  m_tick.start();

  connect(&m_out, &QAudioBufferOutput::audioBufferReceived, this,
          &Spectrum::onBuffer);
  m_timer.setInterval(33);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &Spectrum::tick);
}

void Spectrum::setActive(bool on) {
  m_playing = on;
  apply();
}

void Spectrum::setWanted(bool v) {
  if (m_wanted == v)
    return;
  m_wanted = v;
  emit wantedChanged();
  apply();
}

void Spectrum::apply() {
  const bool on = m_playing && m_wanted;
  if (m_active == on)
    return;
  m_active = on;
  emit activeChanged(on);
  if (on) {
    m_q.clear();
    m_clock.restart();
  }
  m_tick.restart();
  m_timer.start();
}

qint64 Spectrum::estimateMs() const { return m_posMs + m_clock.elapsed(); }

void Spectrum::setPosition(qint64 ms) {
  if (qAbs(ms - estimateMs()) > kDriftMs) {
    m_posMs = ms;
    m_clock.restart();
  }
}

template <class T, class Conv>
void Spectrum::push(const QAudioBuffer &b, int ch, Conv conv) {
  const T *p = b.constData<T>();
  const int n = b.frameCount();
  for (int i = 0; i < n; ++i) {
    float s = 0.0f;
    for (int c = 0; c < ch; ++c)
      s += conv(p[i * ch + c]);
    m_q.push_back(s / ch);
  }
}

void Spectrum::onBuffer(const QAudioBuffer &b) {
  if (!b.isValid() || !m_active)
    return;
  const QAudioFormat f = b.format();
  m_rate = std::max(8000, f.sampleRate());
  const int ch = std::max(1, f.channelCount());
  const qint64 st = b.startTime();
  m_sync = st >= 0;

  if (m_sync) {
    const qint64 expected = m_firstUs + qint64(m_q.size()) * 1000000 / m_rate;
    if (m_q.empty() || qAbs(st - expected) > 20000) {
      m_q.clear();
      m_firstUs = st;
    }
  }

  switch (f.sampleFormat()) {
  case QAudioFormat::UInt8:
    push<quint8>(b, ch, [](quint8 v) { return (float(v) - 128.0f) / 128.0f; });
    break;
  case QAudioFormat::Int16:
    push<qint16>(b, ch, [](qint16 v) { return float(v) / 32768.0f; });
    break;
  case QAudioFormat::Int32:
    push<qint32>(b, ch, [](qint32 v) { return float(v) / 2147483648.0f; });
    break;
  case QAudioFormat::Float:
    push<float>(b, ch, [](float v) { return v; });
    break;
  default:
    break;
  }

  const qint64 cap = qint64(m_rate) * 8;
  if (qint64(m_q.size()) > cap) {
    const qint64 drop = qint64(m_q.size()) - cap;
    m_q.erase(m_q.begin(), m_q.begin() + drop);
    m_firstUs += drop * 1000000 / m_rate;
  }
}

void Spectrum::fft() {
  for (int len = 2; len <= kFft; len <<= 1) {
    const int half = len / 2;
    const int step = kFft / len;
    for (int i = 0; i < kFft; i += len)
      for (int j = 0; j < half; ++j) {
        const auto u = m_x[i + j];
        const auto v = m_x[i + j + half] * m_tw[j * step];
        m_x[i + j] = u + v;
        m_x[i + j + half] = u - v;
      }
  }
}

void Spectrum::smooth(const std::array<float, kBands> &target, float dt) {
  const float disp = 1.0f - std::exp(-dt / kDisplayTau);
  for (int b = 0; b < kBands; ++b) {
    const float tau = target[b] > m_raw[b] ? kAttack[b] : kRelease[b];
    m_raw[b] += (target[b] - m_raw[b]) * (1.0f - std::exp(-dt / tau));
    if (m_raw[b] < 0.003f && target[b] == 0.0f)
      m_raw[b] = 0.0f;
    qreal &lv = m_levels[b];
    lv += (qreal(m_raw[b]) - lv) * disp;
    if (lv < 0.004 && m_raw[b] == 0.0f)
      lv = 0.0;
  }
}

void Spectrum::analyze(float dt) {
  std::array<float, kBands> target{};
  qint64 n = qint64(m_q.size());
  if (n == 0) {
    smooth(target, dt);
    return;
  }

  qint64 end = n;
  if (m_sync) {
    const qint64 nowUs = estimateMs() * 1000;
    end = std::clamp<qint64>((nowUs - m_firstUs) * m_rate / 1000000, 0, n);
  }

  const qint64 old = end - kFft * 2;
  if (old > 0) {
    m_q.erase(m_q.begin(), m_q.begin() + old);
    m_firstUs += old * 1000000 / m_rate;
    end -= old;
    n -= old;
  }

  const qint64 start = end - kFft;
  for (int i = 0; i < kFft; ++i) {
    const qint64 idx = start + i;
    const float s = (idx >= 0 && idx < n) ? m_q[idx] : 0.0f;
    m_x[m_rev[i]] = m_win[i] * s;
  }
  fft();

  const float lo = 60.0f;
  const float hi = std::min(9000.0f, m_rate * 0.45f);
  const float binHz = float(m_rate) / kFft;

  for (int b = 0; b < kBands; ++b) {
    const float f0 = lo * std::pow(hi / lo, float(b) / kBands);
    const float f1 = lo * std::pow(hi / lo, float(b + 1) / kBands);
    const int k0 = std::clamp(int(f0 / binHz), 1, kFft / 2 - 2);
    const int k1 = std::clamp(int(f1 / binHz), k0 + 1, kFft / 2 - 1);

    float sum = 0.0f;
    for (int k = k0; k <= k1; ++k)
      sum += std::norm(m_x[k]);
    const float rms = std::sqrt(sum / float(k1 - k0 + 1));
    const float db = 20.0f * std::log10(rms * 4.0f / kFft + 1e-7f);
    m_peak[b] = std::max(std::max(db, -42.0f), m_peak[b] - kPeakFall[b]);

    float norm =
        std::clamp((db - (m_peak[b] - kRange[b])) / kRange[b], 0.0f, 1.0f);
    target[b] = std::pow(norm, kCurve[b]);
  }
  smooth(target, dt);
}
void Spectrum::tick() {
  const float dt = std::clamp(float(m_tick.restart()) / 1000.0f, 0.001f, 0.05f);
  if (m_active) {
    analyze(dt);
  } else {
    smooth(std::array<float, kBands>{}, dt);
    if (std::none_of(m_levels.begin(), m_levels.end(),
                     [](qreal v) { return v > 0.0; }))
      m_timer.stop();
  }
  bool changed = false;
  for (int b = 0; b < kBands; ++b)
    if (std::abs(m_levels[b] - m_sent[b]) > 0.01 ||
        (m_levels[b] == 0.0 && m_sent[b] != 0.0))
      changed = true;
  if (!changed)
    return;
  for (int b = 0; b < kBands; ++b)
    m_sent[b] = m_levels[b];
  emit levelsChanged();
}
