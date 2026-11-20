#pragma once
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Store : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  Q_INVOKABLE QVariant get(const QString &key,
                           const QVariant &def = QVariant()) const {
    return m_settings.value(key, def);
  }

  Q_INVOKABLE void set(const QString &key, const QVariant &value) {
    m_settings.setValue(key, value);
  }

  static QString readLastUrl() {
    return QSettings().value("lastUrl").toString();
  }

  static qint64 readLastPosition() {
    return QSettings().value("lastPos", 0).toLongLong();
  }

  static void writePlayback(const QString &url, qint64 pos) {
    QSettings s;
    s.setValue("lastUrl", url);
    s.setValue("lastPos", pos);
  }

  static QJsonArray readLibrary() {
    QFile f(libraryPath());
    if (!f.open(QIODevice::ReadOnly))
      return {};
    return QJsonDocument::fromJson(f.readAll()).array();
  }

  static void writeLibrary(const QJsonArray &arr) {
    QFile f(libraryPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
      f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
  }

  static QJsonArray readPlaylists() {
    QFile f(playlistsPath());
    if (!f.open(QIODevice::ReadOnly))
      return {};
    return QJsonDocument::fromJson(f.readAll()).array();
  }

  static void writePlaylists(const QJsonArray &arr) {
    QFile f(playlistsPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
      f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
  }

private:
  static QString libraryPath() {
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/library.json");
  }

  static QString playlistsPath() {
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/playlists.json");
  }

  QSettings m_settings;
};
