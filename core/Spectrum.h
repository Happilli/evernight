#pragma once
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <array>
#include <complex>
#include <deque>

class Spectrum : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("owned by Controller")
  Q_PROPERTY(QList<qreal> levels READ levels NOTIFY levelsChanged)
  Q_PROPERTY(bool wanted READ wanted WRITE setWanted NOTIFY wantedChanged)

public:
  static constexpr int kFft = 1024;
  static constexpr int kBands = 3;

  explicit Spectrum(QObject *parent = nullptr);

  QAudioBufferOutput *output() { return &m_out; }
  QList<qreal> levels() const { return m_levels; }
  void setActive(bool on);
  bool wanted() const { return m_wanted; }
  void setWanted(bool v);
  void setPosition(qint64 ms);

signals:
  void activeChanged(bool on);
  void levelsChanged();
  void wantedChanged();

private:
  void apply();
  bool m_playing = false;
  bool m_wanted = true;
  void onBuffer(const QAudioBuffer &b);
  void tick();
  void analyze(float dt);
  void smooth(const std::array<float, kBands> &target, float dt);
  void fft();
  qint64 estimateMs() const;
  template <class T, class Conv>
  void push(const QAudioBuffer &b, int ch, Conv conv);

  QAudioBufferOutput m_out;
  QTimer m_timer;
  QElapsedTimer m_clock;
  QElapsedTimer m_tick;
  bool m_active = false;
  bool m_sync = false;
  int m_rate = 44100;
  qint64 m_posMs = 0;
  qint64 m_firstUs = 0;
  std::deque<float> m_q;
  std::array<float, kFft> m_win{};
  std::array<int, kFft> m_rev{};
  std::array<std::complex<float>, kFft / 2> m_tw{};
  std::array<std::complex<float>, kFft> m_x{};
  std::array<float, kBands> m_peak{};
  std::array<float, kBands> m_raw{};
  std::array<qreal, kBands> m_sent{};
  QList<qreal> m_levels;
};
