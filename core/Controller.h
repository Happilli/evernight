#pragma once
#include "./Spectrum.h"
#include "DiscordPresence.h"
#include "MediaInfo.h"
#include "Player.h"
#include <QList>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Controller : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(Player *player READ player CONSTANT)
  Q_PROPERTY(MediaInfo *info READ info CONSTANT)
  Q_PROPERTY(Spectrum *spectrum READ spectrum CONSTANT)
  Q_PROPERTY(DiscordPresence *discord READ discord CONSTANT)
  Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)
  Q_PROPERTY(int current READ current NOTIFY currentChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
  Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)
  Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY
                 repeatModeChanged)
  Q_PROPERTY(
      bool autoNext READ autoNext WRITE setAutoNext NOTIFY autoNextChanged)
  Q_PROPERTY(QVariantList recent READ recent NOTIFY recentChanged)

public:
  explicit Controller(QObject *parent = nullptr);

  Player *player() { return &m_player; }
  MediaInfo *info() { return &m_info; }
  Spectrum *spectrum() { return &m_spectrum; }
  DiscordPresence *discord() { return &m_discord; }
  QVariantList tracks() const;
  QVariantList recent() const;
  int current() const { return m_current; }
  bool playing() const {
    return m_player.state() == QMediaPlayer::PlayingState;
  }
  bool shuffle() const { return m_shuffle; }
  int repeatMode() const { return m_repeatMode; }
  bool autoNext() const { return m_autoNext; }

  void setShuffle(bool value);
  void setRepeatMode(int value);
  void setAutoNext(bool value);

  Q_INVOKABLE void playAt(int index);
  Q_INVOKABLE void playQueue(const QStringList &uris, int start);
  Q_INVOKABLE void toggle();
  Q_INVOKABLE void next();
  Q_INVOKABLE void prev();
  Q_INVOKABLE void scanLibrary();
  Q_INVOKABLE void skipPrev();
  Q_INVOKABLE QVariantList search(const QString &query);
  Q_INVOKABLE void inspect(const QString &uri);

signals:
  void inspected(const QString &uri, const QVariantList &rows);
  void tracksChanged();
  void recentChanged();
  void currentChanged();
  void playingChanged();
  void shuffleChanged();
  void repeatModeChanged();
  void autoNextChanged();

private:
  struct Norm {
    QStringList title, artist;
    QString titleFull;
  };
  QList<Norm> m_norm;
  bool m_normDirty = true;
  struct Track {
    QUrl url;
    QString title;
    QString artist;
    QString thumb;
  };

  void importLibrary();
  void loadCache();
  void saveCache() const;
  void restoreState();
  void saveState(bool fromStart = false);
  void play(int index);
  void syncDiscord();
  int queueSize() const;
  int queueAt(int pos) const;
  int queuePos() const;
  void onEnded();

  Player m_player;
  MediaInfo m_info;
  Spectrum m_spectrum;
  DiscordPresence m_discord;
  QList<Track> m_tracks;
  QStringList m_recent;
  QList<int> m_queue;
  int m_current = -1;
  bool m_shuffle = false;
  int m_repeatMode = 0;
  bool m_autoNext = true;
  bool m_awaitingPermission = false;
  bool m_scanning = false;
  qint64 m_pendingSeek = -1;
  qint64 m_savedPos = 0;
};
