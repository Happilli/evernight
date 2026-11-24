#include "./Player.h"

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#endif

namespace {

#ifdef Q_OS_ANDROID
int openDescriptor(const QUrl &url) {
  QJniEnvironment env;
  auto failed = [&env]() {
    if (!env->ExceptionCheck())
      return false;
    env->ExceptionClear();
    return true;
  };
  QJniObject context = QNativeInterface::QAndroidApplication::context();
  if (!context.isValid())
    return -1;
  QJniObject resolver = context.callObjectMethod(
      "getContentResolver", "()Landroid/content/ContentResolver;");
  if (failed() || !resolver.isValid())
    return -1;
  QJniObject uri = QJniObject::callStaticObjectMethod(
      "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
      QJniObject::fromString(QString::fromLatin1(url.toEncoded())).object());
  if (failed() || !uri.isValid())
    return -1;
  QJniObject descriptor = resolver.callObjectMethod(
      "openFileDescriptor",
      "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
      uri.object(), QJniObject::fromString(QStringLiteral("r")).object());
  if (failed() || !descriptor.isValid())
    return -1;
  const jint fd = descriptor.callMethod<jint>("detachFd");
  if (failed())
    return -1;
  return fd;
}
#endif

} // namespace

Player::Player(QObject *parent) : QObject(parent) {
  m_player.setAudioOutput(&m_audio);
  connect(&m_player, &QMediaPlayer::metaDataChanged, this,
          &Player::metaDataChanged);
  connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
          &Player::statusChanged);
  connect(&m_player, &QMediaPlayer::playbackStateChanged, this,
          &Player::stateChanged);
  connect(&m_player, &QMediaPlayer::errorChanged, this, &Player::errorChanged);
  connect(&m_player, &QMediaPlayer::positionChanged, this, [this](qint64 p) {
    if (qAbs(p - m_lastPos) >= 500 || p < m_lastPos) {
      m_lastPos = p;
      emit positionChanged();
    }
  });
  connect(&m_player, &QMediaPlayer::durationChanged, this,
          &Player::durationChanged);
  connect(&m_player, &QMediaPlayer::mediaStatusChanged, this,
          [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia)
              emit ended();
          });
}

QString Player::errorString() const {
  return m_error.isEmpty() ? m_player.errorString() : m_error;
}

bool Player::fail(const QString &message) {
  m_error = message;
  emit errorChanged();
  return false;
}

std::unique_ptr<QFile> Player::openDevice(const QUrl &url) {
  auto file = std::make_unique<QFile>();
  if (url.isLocalFile()) {
    file->setFileName(url.toLocalFile());
    if (!file->open(QIODevice::ReadOnly))
      return nullptr;
    return file;
  }
#ifdef Q_OS_ANDROID
  const int fd = openDescriptor(url);
  if (fd < 0)
    return nullptr;
  if (!file->open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle))
    return nullptr;
  return file;
#else
  return nullptr;
#endif
}

bool Player::open(const QUrl &url) {
  if (!url.isValid() || url.isEmpty())
    return fail(QStringLiteral("invalid url"));
  auto file = openDevice(url);
  if (!file)
    return fail(QStringLiteral("cannot open source"));
  close();
  m_error.clear();
  m_file = std::move(file);
  m_player.setSourceDevice(m_file.get(), url);
  emit errorChanged();
  return true;
}

void Player::close() {
  m_player.stop();
  m_player.setSource(QUrl());
  m_file.reset();
}
