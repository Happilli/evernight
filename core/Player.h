#pragma once
#include <QAudioOutput>
#include <QFile>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <memory>

class Player : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QMediaMetaData metaData READ metaData NOTIFY metaDataChanged)
  Q_PROPERTY(int status READ status NOTIFY statusChanged)
  Q_PROPERTY(int state READ state NOTIFY stateChanged)
  Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
  Q_PROPERTY(
      qint64 position READ position WRITE setPosition NOTIFY positionChanged)
  Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)

public:
  explicit Player(QObject *parent = nullptr);

  Q_INVOKABLE bool open(const QUrl &url);
  Q_INVOKABLE void close();
  Q_INVOKABLE void play() { m_player.play(); }
  Q_INVOKABLE void pause() { m_player.pause(); }
  Q_INVOKABLE void stop() { m_player.stop(); }

  std::unique_ptr<QFile> openDevice(const QUrl &url);

  QMediaMetaData metaData() const { return m_player.metaData(); }
  int status() const { return m_player.mediaStatus(); }
  int state() const { return m_player.playbackState(); }
  QString errorString() const;
  qint64 position() const { return m_player.position(); }
  qint64 duration() const { return m_player.duration(); }
  void setPosition(qint64 position) { m_player.setPosition(position); }

signals:
  void metaDataChanged();
  void statusChanged();
  void stateChanged();
  void errorChanged();
  void positionChanged();
  void durationChanged();
  void ended();

private:
  bool fail(const QString &message);

  QMediaPlayer m_player;
  QAudioOutput m_audio;
  std::unique_ptr<QFile> m_file;
  QString m_error;
  qint64 m_lastPos = 0;
};
