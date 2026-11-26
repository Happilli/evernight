#include "./Controller.h"
#include "./Store.h"
#include <QDate>
#include <QDateTime>
#include <QGuiApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMediaFormat>
#include <QMediaPlayer>
#include <QRandomGenerator>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QtGlobal>
#include <algorithm>
#include <memory>

#ifdef Q_OS_ANDROID
#include <QDir>
#include <QFile>
#include <QJniEnvironment>
#include <QJniObject>
#include <QPointer>
#include <QStandardPaths>
#endif

namespace {

QString fold(const QString &s) {
  const QString n = s.normalized(QString::NormalizationForm_D).toLower();
  QString out;
  out.reserve(n.size());
  bool space = true;
  for (QChar c : n) {
    if (c.category() == QChar::Mark_NonSpacing)
      continue;
    if (c.isLetterOrNumber()) {
      out += c;
      space = false;
    } else if (!space) {
      out += QLatin1Char(' ');
      space = true;
    }
  }
  return out.trimmed();
}

int editDistance(QStringView a, QStringView b, int maxD) {
  if (qAbs(a.size() - b.size()) > maxD)
    return maxD + 1;
  QList<int> prev(b.size() + 1), cur(b.size() + 1);
  for (int j = 0; j <= b.size(); ++j)
    prev[j] = j;
  for (int i = 1; i <= a.size(); ++i) {
    cur[0] = i;
    int rowMin = cur[0];
    for (int j = 1; j <= b.size(); ++j) {
      const int v = qMin(qMin(prev[j] + 1, cur[j - 1] + 1),
                         prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1));
      cur[j] = v;
      rowMin = qMin(rowMin, v);
    }
    if (rowMin > maxD)
      return maxD + 1;
    std::swap(prev, cur);
  }
  return prev[b.size()];
}

int tokenScore(const QString &tok, const QStringList &words) {
  int best = 0;
  const int maxD = tok.size() >= 7 ? 2 : 1;
  for (const QString &w : words) {
    int s = 0;
    if (w == tok)
      s = 100;
    else if (w.startsWith(tok))
      s = 80 - qMin(10, int(w.size() - tok.size()));
    else if (tok.size() >= 2 && w.contains(tok))
      s = 40;
    else if (tok.size() >= 4 &&
             editDistance(QStringView(w).left(tok.size()), tok, maxD) <= maxD)
      s = 30 - 5 * editDistance(QStringView(w).left(tok.size()), tok, maxD);
    best = qMax(best, s);
  }
  return best;
}

QVariantList describe(const QMediaMetaData &m, qint64 durationMs) {
  QVariantList rows;
  auto add = [&rows](const QString &label, const QString &value) {
    if (!value.trimmed().isEmpty())
      rows.append(QVariantMap{{"label", label}, {"value", value.trimmed()}});
  };
  auto str = [&m](QMediaMetaData::Key k) { return m.stringValue(k); };
  add("Title", str(QMediaMetaData::Title));
  add("Artist", str(QMediaMetaData::ContributingArtist));
  add("Album", str(QMediaMetaData::AlbumTitle));
  add("Album artist", str(QMediaMetaData::AlbumArtist));
  add("Genre", str(QMediaMetaData::Genre));
  const QVariant track = m.value(QMediaMetaData::TrackNumber);
  if (track.isValid() && track.toInt() > 0)
    add("Track", QString::number(track.toInt()));
  const QDate d = m.value(QMediaMetaData::Date).toDate();
  if (d.isValid())
    add("Year", QString::number(d.year()));
  if (durationMs > 0) {
    const int s = int(durationMs / 1000);
    add("Duration", QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10,
                                                            QLatin1Char('0')));
  }
  const QVariant codec = m.value(QMediaMetaData::AudioCodec);
  if (codec.isValid())
    add("Codec",
        QMediaFormat::audioCodecName(QMediaFormat::AudioCodec(codec.toInt())));
  const QVariant fmt = m.value(QMediaMetaData::FileFormat);
  if (fmt.isValid())
    add("Format",
        QMediaFormat::fileFormatName(QMediaFormat::FileFormat(fmt.toInt())));
  const QVariant br = m.value(QMediaMetaData::AudioBitRate);
  if (br.isValid() && br.toInt() > 0)
    add("Bitrate",
        QString::number(br.toInt() / 1000) + QStringLiteral(" kbps"));
  add("Comment", str(QMediaMetaData::Comment));
  return rows;
}

#ifdef Q_OS_ANDROID
constexpr int kSdkNotifications = 33;
constexpr int kThumbSize = 512;
constexpr qint64 kSeekJumpMs = 1500;

enum Command { CmdPlay, CmdPause, CmdNext, CmdPrev, CmdSeek };

QPointer<Controller> g_ctl;

QJniObject appContext() {
  return QNativeInterface::QAndroidApplication::context();
}

int sdkVersion() { return QNativeInterface::QAndroidApplication::sdkVersion(); }

jobjectArray stringArray(QJniEnvironment &env, const QStringList &items) {
  jobjectArray arr = env->NewObjectArray(
      items.size(), env->FindClass("java/lang/String"), nullptr);
  for (int i = 0; i < items.size(); ++i)
    env->SetObjectArrayElement(
        arr, i, QJniObject::fromString(items[i]).object<jstring>());
  return arr;
}

void updateService(const QString &title, const QString &artist,
                   const QString &thumb, qint64 duration, qint64 position,
                   bool playing) {
  QJniObject ctx = appContext();
  if (!ctx.isValid())
    return;
  QJniEnvironment env;
  QJniObject::callStaticMethod<void>(
      "com/safalski/evernight/PlaybackService", "update",
      "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;"
      "Ljava/lang/String;JJZ)V",
      ctx.object(), QJniObject::fromString(title).object<jstring>(),
      QJniObject::fromString(artist).object<jstring>(),
      QJniObject::fromString(thumb).object<jstring>(), jlong(duration),
      jlong(position), jboolean(playing));
  env.checkAndClearExceptions();
}

void onCommand(JNIEnv *, jclass, jint cmd, jlong arg) {
  QMetaObject::invokeMethod(
      qGuiApp,
      [cmd, arg] {
        Controller *c = g_ctl;
        if (!c)
          return;
        switch (cmd) {
        case CmdPlay:
          if (!c->playing())
            c->toggle();
          break;
        case CmdPause:
          if (c->playing())
            c->toggle();
          break;
        case CmdNext:
          c->next();
          break;
        case CmdPrev:
          c->prev();
          break;
        case CmdSeek:
          c->player()->setPosition(arg);
          break;
        default:
          break;
        }
      },
      Qt::QueuedConnection);
}

void registerNatives() {
  static const JNINativeMethod methods[] = {
      {"nativeCommand", "(IJ)V", reinterpret_cast<void *>(onCommand)}};
  QJniEnvironment env;
  if (!env.registerNativeMethods("com/safalski/evernight/PlaybackService",
                                 methods,
                                 int(sizeof(methods) / sizeof(methods[0]))))
    qWarning("PlaybackService natives were not registered");
}

QString audioPermission() {
  return sdkVersion() >= kSdkNotifications
             ? QStringLiteral("android.permission.READ_MEDIA_AUDIO")
             : QStringLiteral("android.permission.READ_EXTERNAL_STORAGE");
}

QString notificationPermission() {
  return QStringLiteral("android.permission.POST_NOTIFICATIONS");
}

bool hasPermission(const QString &name) {
  return appContext().callMethod<jint>(
             "checkSelfPermission", "(Ljava/lang/String;)I",
             QJniObject::fromString(name).object<jstring>()) == 0;
}

QStringList missingPermissions() {
  QStringList missing;
  if (!hasPermission(audioPermission()))
    missing << audioPermission();
  if (sdkVersion() >= kSdkNotifications &&
      !hasPermission(notificationPermission()))
    missing << notificationPermission();
  return missing;
}

void requestPermissions(const QStringList &names) {
  if (names.isEmpty())
    return;
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([names] {
    QJniEnvironment env;
    jobjectArray arr = stringArray(env, names);
    appContext().callMethod<void>("requestPermissions",
                                  "([Ljava/lang/String;I)V", arr, 1);
    env.checkAndClearExceptions();
  });
}

void trimBars(const QString &path) {
  QImage img(path);
  if (img.isNull())
    return;
  img = img.convertToFormat(QImage::Format_RGB32);
  auto dark = [&](int y) {
    const QRgb *l = reinterpret_cast<const QRgb *>(img.constScanLine(y));
    for (int x = 0; x < img.width(); ++x)
      if (qMax(qRed(l[x]), qMax(qGreen(l[x]), qBlue(l[x]))) > 16)
        return false;
    return true;
  };
  int top = 0, bot = img.height() - 1;
  while (top < bot && dark(top))
    ++top;
  while (bot > top && dark(bot))
    --bot;
  const int h = bot - top + 1;
  if (h == img.height() || h < img.height() / 2)
    return;
  img.copy(0, top, img.width(), h).save(path, "JPG", 90);
}

QString thumbFor(const QJniObject &ctx, qint64 id) {
  const QString dir =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      QStringLiteral("/thumbs");
  const QString path =
      dir + QLatin1Char('/') + QString::number(id) + QStringLiteral(".jpg");
  if (QFile::exists(path))
    return path;
  QDir().mkpath(dir);
  QJniEnvironment env;
  QJniObject base = QJniObject::getStaticObjectField(
      "android/provider/MediaStore$Audio$Media", "EXTERNAL_CONTENT_URI",
      "Landroid/net/Uri;");
  QJniObject uri = QJniObject::callStaticObjectMethod(
      "android/content/ContentUris", "withAppendedId",
      "(Landroid/net/Uri;J)Landroid/net/Uri;", base.object(), jlong(id));
  QJniObject size("android/util/Size", "(II)V", kThumbSize, kThumbSize);
  QJniObject resolver = ctx.callObjectMethod(
      "getContentResolver", "()Landroid/content/ContentResolver;");
  QJniObject bitmap = resolver.callObjectMethod(
      "loadThumbnail",
      "(Landroid/net/Uri;Landroid/util/Size;Landroid/os/CancellationSignal;)"
      "Landroid/graphics/Bitmap;",
      uri.object(), size.object(), jobject(nullptr));
  if (env.checkAndClearExceptions() || !bitmap.isValid())
    return {};
  QJniObject format = QJniObject::getStaticObjectField(
      "android/graphics/Bitmap$CompressFormat", "JPEG",
      "Landroid/graphics/Bitmap$CompressFormat;");
  QJniObject out("java/io/FileOutputStream", "(Ljava/lang/String;)V",
                 QJniObject::fromString(path).object<jstring>());
  if (env.checkAndClearExceptions() || !out.isValid())
    return {};
  bitmap.callMethod<jboolean>("compress",
                              "(Landroid/graphics/Bitmap$CompressFormat;I"
                              "Ljava/io/OutputStream;)Z",
                              format.object(), jint(85), out.object());
  out.callMethod<void>("close", "()V");
  if (env.checkAndClearExceptions())
    return {};
  trimBars(path);
  return path;
}

struct Row {
  QUrl url;
  QString title;
  QString artist;
  QString thumb;
  QString album;
};

QList<Row> queryMusicFolder() {
  QList<Row> rows;
  QJniEnvironment env;
  QJniObject ctx = appContext();
  QJniObject resolver = ctx.callObjectMethod(
      "getContentResolver", "()Landroid/content/ContentResolver;");
  QJniObject base = QJniObject::getStaticObjectField(
      "android/provider/MediaStore$Audio$Media", "EXTERNAL_CONTENT_URI",
      "Landroid/net/Uri;");
  jobjectArray projection =
      stringArray(env, {QStringLiteral("_id"), QStringLiteral("title"),
                        QStringLiteral("artist"), QStringLiteral("album")});
  QJniObject cursor = resolver.callObjectMethod(
      "query",
      "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/"
      "String;Ljava/lang/String;)Landroid/database/Cursor;",
      base.object(), projection,
      QJniObject::fromString("is_music != 0 AND relative_path LIKE 'Music/%'")
          .object<jstring>(),
      jobjectArray(nullptr),
      QJniObject::fromString("title COLLATE NOCASE ASC").object<jstring>());
  if (env.checkAndClearExceptions() || !cursor.isValid())
    return rows;
  while (cursor.callMethod<jboolean>("moveToNext", "()Z")) {
    const qint64 id = cursor.callMethod<jlong>("getLong", "(I)J", 0);
    const QString title =
        cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", 1)
            .toString();
    QString artist =
        cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", 2)
            .toString();
    if (artist == QStringLiteral("<unknown>"))
      artist.clear();
    QString album =
        cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", 3)
            .toString();
    if (album == QStringLiteral("<unknown>") ||
        album == QStringLiteral("Music"))
      album.clear();
    rows.append({QUrl(QStringLiteral("content://media/external/audio/media/") +
                      QString::number(id)),
                 title, artist, thumbFor(ctx, id), album});
  }
  cursor.callMethod<void>("close", "()V");
  env.checkAndClearExceptions();
  return rows;
}
#endif

} // namespace

Controller::Controller(QObject *parent) : QObject(parent) {
  connect(&m_player, &Player::metaDataChanged, this,
          [this] { m_info.setMetaData(m_player.metaData()); });
  connect(&m_player, &Player::stateChanged, this, [this] {
    if (m_player.state() == QMediaPlayer::PlayingState)
      m_switching = false;
    else if (m_switching)
      return;
    emit playingChanged();
  });
  connect(&m_player, &Player::statusChanged, this, [this] {
    if (!m_switching || m_player.status() != QMediaPlayer::InvalidMedia)
      return;
    m_switching = false;
    emit playingChanged();
  });
  connect(&m_player, &Player::ended, this, &Controller::onEnded);
  connect(&m_player, &Player::statusChanged, this, [this] {
    if (m_pendingSeek < 0)
      return;
    const int s = m_player.status();
    if (s != QMediaPlayer::LoadedMedia && s != QMediaPlayer::BufferedMedia)
      return;
    m_player.setPosition(m_pendingSeek);
    m_pendingSeek = -1;
  });
  connect(&m_player, &Player::positionChanged, this, [this] {
    if (m_pendingSeek >= 0)
      return;
    if (qAbs(m_player.position() - m_savedPos) >= 5000)
      saveState();
  });
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
          [this](Qt::ApplicationState state) {
            if (state != Qt::ApplicationActive)
              saveState();
          });
  connect(this, &Controller::playingChanged, this, &Controller::syncDiscord);
  connect(this, &Controller::currentChanged, this, &Controller::syncDiscord);
  connect(&m_info, &MediaInfo::changed, this, &Controller::syncDiscord);
  connect(&m_player, &Player::durationChanged, this, &Controller::syncDiscord);
  connect(&m_player, &Player::positionChanged, this,
          [this, last = qint64(0)]() mutable {
            const qint64 p = m_player.position();
            if (qAbs(p - last) > 1500)
              syncDiscord();
            last = p;
          });
  connect(this, &Controller::tracksChanged, this,
          [this] { m_normDirty = true; });
  m_sleepTimer.setInterval(250);
  connect(&m_sleepTimer, &QTimer::timeout, this, &Controller::onSleepTick);
  m_autoNext = QSettings().value("autoNext", true).toBool();
  m_shuffle = QSettings().value("shuffle", false).toBool();
  m_repeatMode = qBound(0, QSettings().value("repeatMode", 0).toInt(), 2);
  QTimer::singleShot(0, this, [this] {
    m_discord.setEnabled(QSettings().value("discord", false).toBool());
  });
#ifdef Q_OS_ANDROID
  g_ctl = this;
  registerNatives();
  auto push = [this] {
    if (m_current < 0 || m_current >= m_tracks.size())
      return;
    const Track &t = m_tracks[m_current];
    updateService(m_info.title().isEmpty() ? t.title : m_info.title(),
                  m_info.artist().isEmpty() ? t.artist : m_info.artist(),
                  t.thumb, m_player.duration(), m_player.position(), playing());
  };
  connect(&m_player, &Player::durationChanged, this, push);
  connect(&m_player, &Player::metaDataChanged, this, push);
  connect(&m_player, &Player::stateChanged, this, push);
  connect(this, &Controller::currentChanged, this, push);
  connect(this, &Controller::queueChanged, this, [this] {
    m_shufflePos = -1;
    emit neighborsChanged();
  });
  connect(this, &Controller::shuffleChanged, this,
          &Controller::neighborsChanged);
  connect(this, &Controller::tracksChanged, this,
          &Controller::neighborsChanged);
  connect(&m_player, &Player::positionChanged, this,
          [this, push, last = qint64(0)]() mutable {
            const qint64 p = m_player.position();
            if (qAbs(p - last) > kSeekJumpMs)
              push();
            last = p;
          });
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
          [this](Qt::ApplicationState state) {
            if (!m_awaitingPermission || state != Qt::ApplicationActive)
              return;
            if (!hasPermission(audioPermission()))
              return;
            m_awaitingPermission = false;
            importLibrary();
          });
#endif
  m_recent = QSettings().value("recent").toStringList();
  connect(this, &Controller::tracksChanged, this, &Controller::recentChanged);
  connect(this, &Controller::tracksChanged, this, &Controller::queueChanged);
  connect(this, &Controller::currentChanged, this, [this] {
    m_info.setThumb(m_current >= 0 && m_current < m_tracks.size()
                        ? m_tracks[m_current].thumb
                        : QString());
  });
  loadCache();
  restoreState();
  QTimer::singleShot(0, this, &Controller::scanLibrary);
}

void Controller::syncDiscord() {
  if (m_current < 0 || m_current >= m_tracks.size()) {
    m_discord.clear();
    return;
  }
  const Track &t = m_tracks[m_current];
  m_discord.update(m_info.title().isEmpty() ? t.title : m_info.title(),
                   m_info.artist().isEmpty() ? t.artist : m_info.artist(),
                   m_player.duration(), m_player.position(), playing());
}

void Controller::inspect(const QString &uri) {
  const QUrl url(uri);
  if (m_current >= 0 && m_current < m_tracks.size() &&
      m_tracks[m_current].url == url && m_player.duration() > 0) {
    emit inspected(uri, describe(m_player.metaData(), m_player.duration()));
    return;
  }
  std::shared_ptr<QFile> file(m_player.openDevice(url).release());
  if (!file) {
    emit inspected(uri, {});
    return;
  }
  auto *mp = new QMediaPlayer(this);
  mp->setSourceDevice(file.get(), url);
  auto done = std::make_shared<bool>(false);
  auto finish = [this, mp, file, done, uri] {
    if (*done)
      return;
    *done = true;
    emit inspected(uri, describe(mp->metaData(), mp->duration()));
    mp->deleteLater();
  };
  connect(mp, &QMediaPlayer::mediaStatusChanged, mp,
          [finish](QMediaPlayer::MediaStatus s) {
            if (s == QMediaPlayer::LoadedMedia ||
                s == QMediaPlayer::InvalidMedia)
              finish();
          });
  QTimer::singleShot(4000, mp, finish);
}

void Controller::loadCache() {
  for (const QJsonValue &v : Store::readLibrary()) {
    const QJsonObject o = v.toObject();
    m_tracks.append({QUrl(o["url"].toString()), o["title"].toString(),
                     o["artist"].toString(), o["thumb"].toString(),
                     o["album"].toString()});
  }
  if (!m_tracks.isEmpty())
    emit tracksChanged();
}

void Controller::saveCache() const {
  QJsonArray arr;
  for (const Track &t : m_tracks)
    arr.append(QJsonObject{{"url", t.url.toString()},
                           {"title", t.title},
                           {"artist", t.artist},
                           {"thumb", t.thumb},
                           {"album", t.album}});
  Store::writeLibrary(arr);
}

QVariantList Controller::recent() const {
  QHash<QString, int> byUrl;
  for (int i = 0; i < m_tracks.size(); ++i)
    byUrl.insert(m_tracks[i].url.toString(), i);
  QVariantList out;
  for (const QString &u : m_recent) {
    if (out.size() >= 5)
      break;
    const auto it = byUrl.constFind(u);
    if (it != byUrl.constEnd())
      out.append(it.value());
  }
  return out;
}

void Controller::restoreState() {
  const QString url = Store::readLastUrl();
  if (url.isEmpty())
    return;
  for (int i = 0; i < m_tracks.size(); ++i) {
    if (m_tracks[i].url.toString() != url)
      continue;
    const qint64 pos = Store::readLastPosition();
    m_pendingSeek = pos > 0 ? pos : -1;
    if (!m_player.open(m_tracks[i].url)) {
      m_pendingSeek = -1;
      return;
    }
    m_current = i;
    m_savedPos = pos;
    emit currentChanged();
    return;
  }
}

void Controller::saveState(bool fromStart) {
  if (m_current < 0 || m_current >= m_tracks.size())
    return;
  qint64 pos = fromStart ? 0 : m_player.position();
  if (m_pendingSeek >= 0)
    pos = m_pendingSeek;
  m_savedPos = pos;
  Store::writePlayback(m_tracks[m_current].url.toString(), pos);
}

QVariantList Controller::tracks() const {
  QVariantList list;
  list.reserve(m_tracks.size());
  for (const Track &t : m_tracks)
    list.append(QVariantMap{{QStringLiteral("uri"), t.url.toString()},
                            {QStringLiteral("title"), t.title},
                            {QStringLiteral("artist"), t.artist},
                            {QStringLiteral("thumb"), t.thumb},
                            {QStringLiteral("album"), t.album}});
  return list;
}

QVariantList Controller::group(bool byAlbum) const {
  struct G {
    QString key, name, artist, thumb;
    bool various = false;
    QVariantList indices;
    QStringList uris;
  };
  QList<G> groups;
  QHash<QString, int> byKey;
  for (int i = 0; i < m_tracks.size(); ++i) {
    const Track &t = m_tracks[i];
    const QString label = byAlbum ? t.album : t.artist;
    const QString key = fold(label);
    auto it = byKey.find(key);
    if (it == byKey.end()) {
      G g;
      g.key = key;
      g.name = label.isEmpty() ? (byAlbum ? QStringLiteral("Unknown album")
                                          : QStringLiteral("Unknown artist"))
                               : label;
      g.artist = t.artist;
      it = byKey.insert(key, int(groups.size()));
      groups.append(g);
    }
    G &g = groups[it.value()];
    if (g.thumb.isEmpty())
      g.thumb = t.thumb;
    if (byAlbum && !g.various && g.artist != t.artist) {
      g.various = true;
      g.artist = QStringLiteral("Various artists");
    }
    g.indices.append(i);
    g.uris.append(t.url.toString());
  }
  std::stable_sort(groups.begin(), groups.end(), [](const G &a, const G &b) {
    if (a.key.isEmpty() != b.key.isEmpty())
      return !a.key.isEmpty();
    return a.key < b.key;
  });
  QVariantList out;
  out.reserve(groups.size());
  for (const G &g : std::as_const(groups))
    out.append(QVariantMap{{QStringLiteral("name"), g.name},
                           {QStringLiteral("artist"), g.artist},
                           {QStringLiteral("thumb"), g.thumb},
                           {QStringLiteral("count"), int(g.indices.size())},
                           {QStringLiteral("indices"), g.indices},
                           {QStringLiteral("uris"), g.uris}});
  return out;
}

QVariantList Controller::albums() const { return group(true); }

QVariantList Controller::artists() const { return group(false); }

void Controller::setShuffle(bool value) {
  if (m_shuffle == value)
    return;
  m_shuffle = value;
  emit shuffleChanged();
}

void Controller::setRepeatMode(int value) {
  if (m_repeatMode == value)
    return;
  m_repeatMode = value;
  emit repeatModeChanged();
}

void Controller::setAutoNext(bool value) {
  if (m_autoNext == value)
    return;
  m_autoNext = value;
  emit autoNextChanged();
}

int Controller::sleepRemaining() const {
  if (!m_sleepEnd)
    return 0;
  const qint64 left = m_sleepEnd - QDateTime::currentMSecsSinceEpoch();
  return left > 0 ? int((left + 999) / 1000) : 0;
}

void Controller::setSleepTimer(int seconds) {
  if (m_sleepEnd)
    m_player.setVolume(m_sleepBaseVol);
  if (seconds <= 0) {
    m_sleepTimer.stop();
    m_sleepEnd = 0;
  } else {
    if (!m_sleepEnd)
      m_sleepBaseVol = m_player.volume();
    m_sleepEnd = QDateTime::currentMSecsSinceEpoch() + qint64(seconds) * 1000;
    m_sleepFadeMs = qMin<qint64>(15000, qint64(seconds) * 500);
    m_sleepLastSec = -1;
    m_sleepTimer.start();
  }
  emit sleepChanged();
}

void Controller::onSleepTick() {
  const qint64 left = m_sleepEnd - QDateTime::currentMSecsSinceEpoch();
  if (left <= 0) {
    const float base = m_sleepBaseVol;
    m_sleepTimer.stop();
    m_sleepEnd = 0;
    m_player.pause();
    QTimer::singleShot(800, this, [this, base] { m_player.setVolume(base); });
    emit sleepChanged();
    return;
  }
  if (left < m_sleepFadeMs) {
    const qreal t = qreal(left) / qreal(m_sleepFadeMs);
    m_player.setVolume(m_sleepBaseVol * float(t * t));
  }
  const int sec = sleepRemaining();
  if (sec != m_sleepLastSec) {
    m_sleepLastSec = sec;
    emit sleepChanged();
  }
}

int Controller::queueSize() const {
  return m_queue.isEmpty() ? m_tracks.size() : m_queue.size();
}

int Controller::queueAt(int pos) const {
  return m_queue.isEmpty() ? pos : m_queue[pos];
}

int Controller::queuePos() const {
  return m_queue.isEmpty() ? m_current : m_queue.indexOf(m_current);
}

void Controller::play(int index) {
  if (index < 0 || index >= m_tracks.size())
    return;
  m_switching = playing();
  if (!m_player.open(m_tracks[index].url)) {
    m_switching = false;
    return;
  }
  m_pendingSeek = -1;
  m_current = index;
  m_shufflePos = -1;
  emit currentChanged();
  emit neighborsChanged();
  m_player.play();
  const QString u = m_tracks[index].url.toString();
  QTimer::singleShot(400, this, [this, u] {
    m_recent.removeAll(u);
    m_recent.prepend(u);
    while (m_recent.size() > 5)
      m_recent.removeLast();
    QSettings().setValue("recent", m_recent);
    emit recentChanged();
    if (m_current >= 0 && m_current < m_tracks.size() &&
        m_tracks[m_current].url.toString() == u)
      saveState(true);
  });
}

void Controller::playAt(int index) {
  m_queue.clear();
  emit queueChanged();
  play(index);
}

void Controller::playQueue(const QStringList &uris, int start) {
  QHash<QString, int> byUrl;
  byUrl.reserve(m_tracks.size());
  for (int i = 0; i < m_tracks.size(); ++i)
    byUrl.insert(m_tracks[i].url.toString(), i);
  QList<int> queue;
  int first = -1;
  for (int i = 0; i < uris.size(); ++i) {
    const auto it = byUrl.constFind(uris[i]);
    if (it == byUrl.constEnd())
      continue;
    if (i == start)
      first = queue.size();
    queue.append(it.value());
  }
  if (queue.isEmpty())
    return;
  m_queue = queue;
  emit queueChanged();
  play(queue[first >= 0 ? first : 0]);
}

void Controller::toggle() {
  if (m_tracks.isEmpty())
    return;
  if (m_current < 0)
    playAt(0);
  else if (playing())
    m_player.pause();
  else
    m_player.play();
}

int Controller::nextIndex() {
  const int n = queueSize();
  if (n == 0)
    return -1;
  const int pos = queuePos();
  if (m_shuffle && n > 1) {
    if (m_shufflePos < 0 || m_shufflePos >= n || m_shufflePos == pos) {
      int r = pos;
      while (r == pos)
        r = QRandomGenerator::global()->bounded(n);
      m_shufflePos = r;
    }
    return queueAt(m_shufflePos);
  }
  return queueAt((pos + 1) % n);
}

int Controller::prevIndex() {
  const int n = queueSize();
  if (n == 0)
    return -1;
  const int pos = queuePos();
  return queueAt(pos < 0 ? 0 : (pos - 1 + n) % n);
}

void Controller::next() {
  const int t = nextIndex();
  m_shufflePos = -1;
  if (t >= 0)
    play(t);
}

void Controller::skipPrev() {
  const int n = queueSize();
  if (n == 0)
    return;
  const int pos = queuePos();
  play(queueAt(pos < 0 ? 0 : (pos - 1 + n) % n));
}

void Controller::prev() {
  const int n = queueSize();
  if (n == 0)
    return;
  if (m_current < 0 || m_player.position() > 3000) {
    m_player.setPosition(0);
    return;
  }
  const int pos = queuePos();
  play(queueAt(pos < 0 ? 0 : (pos - 1 + n) % n));
}

void Controller::onEnded() {
  if (m_repeatMode == 2) {
    m_player.setPosition(0);
    m_player.play();
    return;
  }
  if (!m_autoNext)
    return;
  if (!m_shuffle && m_repeatMode == 0 && queuePos() >= queueSize() - 1)
    return;
  next();
}

void Controller::importLibrary() {
#ifdef Q_OS_ANDROID
  if (m_scanning)
    return;
  m_scanning = true;
  auto *thread = QThread::create([this] {
    QList<Track> list;
    for (const Row &r : queryMusicFolder())
      list.append({r.url,
                   r.title.isEmpty() ? QStringLiteral("Unknown") : r.title,
                   r.artist, r.thumb, r.album});
    QMetaObject::invokeMethod(
        this,
        [this, list] {
          QUrl playingUrl;
          if (m_current >= 0 && m_current < m_tracks.size())
            playingUrl = m_tracks[m_current].url;
          QStringList queueUrls;
          for (int i : std::as_const(m_queue))
            if (i >= 0 && i < m_tracks.size())
              queueUrls << m_tracks[i].url.toString();
          m_tracks = list;
          m_current = -1;
          QHash<QString, int> byUrl;
          byUrl.reserve(m_tracks.size());
          for (int i = 0; i < m_tracks.size(); ++i) {
            byUrl.insert(m_tracks[i].url.toString(), i);
            if (m_tracks[i].url == playingUrl)
              m_current = i;
          }
          m_queue.clear();
          for (const QString &u : std::as_const(queueUrls)) {
            const auto it = byUrl.constFind(u);
            if (it != byUrl.constEnd())
              m_queue.append(it.value());
          }
          m_scanning = false;
          emit tracksChanged();
          emit currentChanged();
          saveCache();
        },
        Qt::QueuedConnection);
  });
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  thread->start();
#endif
}

void Controller::scanLibrary() {
#ifdef Q_OS_ANDROID
  if (hasPermission(audioPermission()))
    importLibrary();
  else
    m_awaitingPermission = true;
  requestPermissions(missingPermissions());
#endif
}

QVariantList Controller::search(const QString &query) {
  if (m_normDirty) {
    m_norm.clear();
    m_norm.reserve(m_tracks.size());
    for (const Track &t : std::as_const(m_tracks)) {
      const QString ft = fold(t.title);
      m_norm.append({ft.split(' ', Qt::SkipEmptyParts),
                     fold(t.artist).split(' ', Qt::SkipEmptyParts), ft});
    }
    m_normDirty = false;
  }
  QVariantList out;
  const QString q = fold(query);
  if (q.isEmpty()) {
    out.reserve(m_tracks.size());
    for (int i = 0; i < m_tracks.size(); ++i)
      out.append(i);
    return out;
  }
  const QStringList toks = q.split(' ', Qt::SkipEmptyParts);
  QList<QPair<int, int>> scored;
  for (int i = 0; i < m_norm.size(); ++i) {
    const Norm &n = m_norm[i];
    int total = 0;
    bool ok = true;
    for (const QString &tok : toks) {
      const int s =
          qMax(3 * tokenScore(tok, n.title), 2 * tokenScore(tok, n.artist));
      if (s == 0) {
        ok = false;
        break;
      }
      total += s;
    }
    if (!ok)
      continue;
    if (n.titleFull == q)
      total += 200;
    else if (n.titleFull.startsWith(q))
      total += 80;
    else if (n.titleFull.contains(q))
      total += 40;
    scored.append({total, i});
  }
  std::stable_sort(
      scored.begin(), scored.end(),
      [](const auto &a, const auto &b) { return a.first > b.first; });
  out.reserve(scored.size());
  for (const auto &p : scored)
    out.append(p.second);
  return out;
}

QVariantList Controller::queue() const {
  const int n = queueSize();
  QVariantList out;
  out.reserve(n);
  for (int pos = 0; pos < n; ++pos) {
    const int i = queueAt(pos);
    if (i < 0 || i >= m_tracks.size())
      continue;
    const Track &t = m_tracks[i];
    out.append(QVariantMap{{QStringLiteral("uri"), t.url.toString()},
                           {QStringLiteral("title"), t.title},
                           {QStringLiteral("artist"), t.artist},
                           {QStringLiteral("thumb"), t.thumb},
                           {QStringLiteral("album"), t.album}});
  }
  return out;
}

void Controller::playFromQueue(int pos) {
  if (pos < 0 || pos >= queueSize())
    return;
  play(queueAt(pos));
}
