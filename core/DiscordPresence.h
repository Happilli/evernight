#pragma once
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <memory>

class DiscordPresence : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("owned by Controller")
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
public:
  explicit DiscordPresence(QObject *parent = nullptr);
  ~DiscordPresence() override;
  bool enabled() const { return m_enabled; }
  bool connected() const { return m_connected; }
  void setEnabled(bool v);
  Q_INVOKABLE void login();
  Q_INVOKABLE void logout();
  Q_INVOKABLE bool hasToken() const;
  void update(const QString &title, const QString &artist, qint64 durationMs,
              qint64 positionMs, bool playing);
  void clear();
signals:
  void authRequired();
  void enabledChanged();
  void connectedChanged();

private:
  struct Impl;
  void ensureClient();
  void resume();
  void adoptToken(const QString &access, const QString &refresh);
  void flush();
  void setConnected(bool v);
  std::unique_ptr<Impl> d;
  bool m_enabled = false;
  bool m_connected = false;
};
