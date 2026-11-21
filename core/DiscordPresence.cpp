#include "./DiscordPresence.h"
#include <QDateTime>
#include <QHash>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <functional>
#include <thread>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#define DISCORDPP_IMPLEMENTATION
#include <discordpp.h>
#endif

namespace {
constexpr quint64 kAppId = 1573360530175823882ULL;
const QString kRefreshKey = QStringLiteral("discord/refreshToken");
constexpr const char *kIconImage = "evernight";
constexpr const char *kSmallImage = "https://i.imgur.com/9eebw9T.gif";
constexpr const char *kButtonLabel = "Listen along";
constexpr const char *kButtonUrl =
    "https://www.youtube.com/watch?v=dz1MhkbPthI";

void fetchCover(QObject *ctx, const QString &title, const QString &artist,
                std::function<void(QString)> done) {
#ifdef Q_OS_ANDROID
  QString clean = title;
  clean.remove(QRegularExpression(QStringLiteral("\\s*[\\(\\[].*?[\\)\\]]")));
  const QString term = artist + QLatin1Char(' ') + clean.trimmed();
  QPointer<QObject> guard(ctx);
  std::thread([guard, term, done] {
    QJniEnvironment env;
    const QString art = QJniObject::callStaticMethod<jstring>(
                            "com/safalski/evernight/CoverLookup", "find",
                            "(Ljava/lang/String;)Ljava/lang/String;",
                            QJniObject::fromString(term).object<jstring>())
                            .toString();
    if (!guard)
      return;
    QMetaObject::invokeMethod(
        guard.data(), [done, art] { done(art); }, Qt::QueuedConnection);
  }).detach();
#else
  Q_UNUSED(ctx)
  Q_UNUSED(title)
  Q_UNUSED(artist)
  done({});
#endif
}
} // namespace

struct DiscordPresence::Impl {
  QTimer clearTimer;
  QTimer pump;
  QTimer debounce;
  struct {
    QString title, artist;
    qint64 start = 0, end = 0;
    bool valid = false;
    bool paused = false;
  } pending;
  QString sentKey;
  qint64 sentStart = 0;
  bool refreshing = false;
  QHash<QString, QString> covers;
#ifdef Q_OS_ANDROID
  std::shared_ptr<discordpp::Client> client;
#endif
};

DiscordPresence::DiscordPresence(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>()) {
  d->pump.setInterval(16);
  d->debounce.setSingleShot(true);
  d->debounce.setInterval(400);
  connect(&d->debounce, &QTimer::timeout, this, &DiscordPresence::flush);
  d->clearTimer.setSingleShot(true);
  connect(&d->clearTimer, &QTimer::timeout, this, &DiscordPresence::clear);
#ifdef Q_OS_ANDROID
  connect(&d->pump, &QTimer::timeout, this, [] { discordpp::RunCallbacks(); });
#endif
}

DiscordPresence::~DiscordPresence() = default;

bool DiscordPresence::hasToken() const {
  return !QSettings().value(kRefreshKey).toString().isEmpty();
}

void DiscordPresence::setConnected(bool v) {
  if (m_connected == v)
    return;
  m_connected = v;
  emit connectedChanged();
  if (v && d->pending.valid)
    d->debounce.start();
}

void DiscordPresence::setEnabled(bool v) {
  if (m_enabled == v)
    return;
  m_enabled = v;
  emit enabledChanged();
  if (v) {
    resume();
    return;
  }
  clear();
#ifdef Q_OS_ANDROID
  if (d->client)
    d->client->Disconnect();
#endif
  setConnected(false);
}

void DiscordPresence::ensureClient() {
#ifdef Q_OS_ANDROID
  if (d->client)
    return;
  QJniObject activity = QNativeInterface::QAndroidApplication::context();
  QJniObject::callStaticMethod<void>(
      "com/discord/socialsdk/DiscordSocialSdkInit", "setEngineActivity",
      "(Landroid/app/Activity;)V", activity.object());
  d->client = std::make_shared<discordpp::Client>();
  d->client->SetApplicationId(kAppId);
  d->client->AddLogCallback(
      [](std::string msg, discordpp::LoggingSeverity sev) {
        qInfo("DiscordSDK[%d] %s", int(sev), msg.c_str());
      },
      discordpp::LoggingSeverity::Info);
  QPointer<DiscordPresence> self(this);
  d->client->SetStatusChangedCallback([self](discordpp::Client::Status s,
                                             discordpp::Client::Error e,
                                             int32_t detail) {
    qInfo("Discord status=%s err=%d detail=%d",
          discordpp::Client::StatusToString(s).c_str(), int(e), detail);
    if (!self)
      return;
    self->setConnected(s == discordpp::Client::Status::Ready);
  });
  d->client->SetTokenExpirationCallback([self] {
    if (self && self->m_enabled)
      self->resume();
  });
  d->pump.start();
#endif
}

void DiscordPresence::login() {
#ifdef Q_OS_ANDROID
  ensureClient();
  auto verifier = d->client->CreateAuthorizationCodeVerifier();
  discordpp::AuthorizationArgs args;
  args.SetClientId(kAppId);
  args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
  args.SetCodeChallenge(verifier.Challenge());
  QPointer<DiscordPresence> self(this);
  d->client->Authorize(args, [self, verifier](discordpp::ClientResult r,
                                              std::string code,
                                              std::string redirectUri) {
    if (!self)
      return;
    if (!r.Successful()) {
      const auto st = r.Status();
      qWarning("Discord authorize failed: %s (status=%d)", r.Error().c_str(),
               int(st));
      if (st == discordpp::HttpStatusCode::BadRequest) {
        QSettings().remove(kRefreshKey);
        emit self->authRequired();
      }
      return;
    }
    self->d->client->GetToken(
        kAppId, code, verifier.Verifier(), redirectUri,
        [self](discordpp::ClientResult r, std::string access,
               std::string refresh, discordpp::AuthorizationTokenType, int32_t,
               std::string) {
          if (!self)
            return;
          if (!r.Successful()) {
            qWarning("Discord token exchange failed: %s", r.Error().c_str());
            return;
          }
          self->adoptToken(QString::fromStdString(access),
                           QString::fromStdString(refresh));
        });
  });
#endif
}

void DiscordPresence::resume() {
#ifdef Q_OS_ANDROID
  const QString rt = QSettings().value(kRefreshKey).toString();
  if (rt.isEmpty() || d->refreshing)
    return;
  d->refreshing = true;
  ensureClient();
  QPointer<DiscordPresence> self(this);
  d->client->RefreshToken(
      kAppId, rt.toStdString(),
      [self](discordpp::ClientResult r, std::string access, std::string refresh,
             discordpp::AuthorizationTokenType, int32_t, std::string) {
        if (!self)
          return;
        self->d->refreshing = false;
        if (!r.Successful()) {
          const auto st = r.Status();
          qWarning("Discord refresh failed: %s (status=%d)", r.Error().c_str(),
                   int(st));
          if (st == discordpp::HttpStatusCode::BadRequest ||
              st == discordpp::HttpStatusCode::Unauthorized) {
            QSettings().remove(kRefreshKey);
            emit self->authRequired();
          }
          return;
        }
        self->adoptToken(QString::fromStdString(access),
                         QString::fromStdString(refresh));
      });
#endif
}

void DiscordPresence::adoptToken(const QString &access,
                                 const QString &refresh) {
#ifdef Q_OS_ANDROID
  if (!refresh.isEmpty())
    QSettings().setValue(kRefreshKey, refresh);
  QPointer<DiscordPresence> self(this);
  d->client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,
                         access.toStdString(),
                         [self](discordpp::ClientResult r) {
                           qInfo("Discord UpdateToken: %s",
                                 r.Successful() ? "ok" : r.Error().c_str());
                           if (self && r.Successful())
                             self->d->client->Connect();
                         });
#else
  Q_UNUSED(access)
  Q_UNUSED(refresh)
#endif
}

void DiscordPresence::logout() {
  clear();
  QSettings().remove(kRefreshKey);
#ifdef Q_OS_ANDROID
  if (d->client)
    d->client->Disconnect();
#endif
  setConnected(false);
}

void DiscordPresence::update(const QString &title, const QString &artist,
                             qint64 durationMs, qint64 positionMs,
                             bool playing) {
  if (!m_enabled)
    return;
  if (title.size() < 2) {
    d->debounce.stop();
    d->pending.valid = false;
    d->clearTimer.start(2500);
    return;
  }
  if (playing && durationMs <= 0) {
    d->debounce.stop();
    return;
  }
  if (playing)
    d->clearTimer.stop();
  else
    d->clearTimer.start(10 * 60 * 1000);
  const qint64 start = QDateTime::currentMSecsSinceEpoch() - positionMs;
  d->pending = {title, artist,  start, durationMs > 0 ? start + durationMs : 0,
                true,  !playing};
  d->debounce.start();
}

void DiscordPresence::flush() {
  if (!d->pending.valid || !m_connected)
    return;
#ifdef Q_OS_ANDROID
  const auto p = d->pending;
  const QString key = p.title + QChar(0) + p.artist;
  const QString sig = p.paused ? key + QChar(1) : key;
  if (sig == d->sentKey && (p.paused || qAbs(p.start - d->sentStart) < 2500))
    return;
  d->sentKey = sig;
  d->sentStart = p.start;
  qInfo("Discord flush: %s - %s%s", qPrintable(p.title), qPrintable(p.artist),
        p.paused ? " (paused)" : "");

  auto client = d->client;

  auto build = [p](const QString &image, bool withSmall) {
    discordpp::Activity a;
    a.SetType(discordpp::ActivityTypes::Listening);
    a.SetName("Evernight");
    a.SetDetails(p.title.toStdString());
    if (p.paused) {
      const QString s = p.artist.isEmpty()
                            ? QStringLiteral("Paused")
                            : p.artist + QStringLiteral(" \u2022 Paused");
      a.SetState(s.toStdString());
    } else if (!p.artist.isEmpty()) {
      a.SetState(p.artist.toStdString());
    }
    if (!p.artist.isEmpty())
      a.SetStatusDisplayType(discordpp::StatusDisplayTypes::State);
    if (!p.paused) {
      discordpp::ActivityTimestamps ts;
      ts.SetStart(uint64_t(p.start));
      if (p.end > 0)
        ts.SetEnd(uint64_t(p.end));
      a.SetTimestamps(ts);
    }

    discordpp::ActivityAssets assets;
    bool hasAssets = false;
    if (!image.isEmpty()) {
      assets.SetLargeImage(image.toStdString());
      hasAssets = true;
    }
    if (withSmall) {
      assets.SetSmallImage(p.paused ? kIconImage : kSmallImage);
      hasAssets = true;
    }
    if (hasAssets)
      a.SetAssets(assets);

    discordpp::ActivityButton button;
    button.SetLabel(kButtonLabel);
    button.SetUrl(kButtonUrl);
    a.AddButton(button);
    return a;
  };

  auto send = [client, build](const QString &image) {
    client->UpdateRichPresence(
        build(image, true), [client, build, image](discordpp::ClientResult r) {
          if (r.Successful()) {
            qInfo("Discord presence sent");
            return;
          }
          qWarning("Discord presence failed: %s, retrying without small image",
                   r.Error().c_str());
          client->UpdateRichPresence(
              build(image, false), [client, build](discordpp::ClientResult r2) {
                if (r2.Successful()) {
                  qInfo("Discord presence sent (no small image)");
                  return;
                }
                qWarning("Discord presence failed: %s, retrying bare",
                         r2.Error().c_str());
                client->UpdateRichPresence(
                    build({}, false), [](discordpp::ClientResult r3) {
                      if (!r3.Successful())
                        qWarning("Discord presence failed: %s",
                                 r3.Error().c_str());
                    });
              });
        });
  };

  const QString fallback = QString::fromLatin1(kIconImage);

  if (p.artist.isEmpty()) {
    send(fallback);
    return;
  }

  if (d->covers.contains(key)) {
    send(d->covers.value(key));
    return;
  }

  QPointer<DiscordPresence> self(this);
  fetchCover(this, p.title, p.artist,
             [self, key, sig, send, fallback](QString art) {
               if (!self)
                 return;
               const QString image = art.isEmpty() ? fallback : art;
               qInfo("Discord cover: %s", qPrintable(image));
               self->d->covers.insert(key, image);
               if (self->d->sentKey == sig)
                 send(image);
             });
#endif
}

void DiscordPresence::clear() {
  d->clearTimer.stop();
  d->debounce.stop();
  d->pending.valid = false;
  d->sentKey.clear();
#ifdef Q_OS_ANDROID
  if (d->client && m_connected)
    d->client->ClearRichPresence();
#endif
}
