#pragma once
#include "./Store.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QUuid>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Playlists : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QVariantList playlists READ playlists NOTIFY changed)

public:
  explicit Playlists(QObject *parent = nullptr) : QObject(parent) {
    for (const QJsonValue &v : Store::readPlaylists()) {
      const QJsonObject o = v.toObject();
      Playlist p;
      p.id = o["id"].toString();
      p.name = o["name"].toString();
      for (const QJsonValue &t : o["tracks"].toArray())
        p.tracks.append(t.toString());
      if (!p.id.isEmpty() && !p.name.isEmpty())
        m_lists.append(p);
    }

    bool hasLiked = false;
    for (const Playlist &p : m_lists)
      if (p.id == QStringLiteral("liked"))
        hasLiked = true;
    if (!hasLiked) {
      m_lists.prepend(
          {QStringLiteral("liked"), QStringLiteral("Liked songs"), {}});
      save();
    }
  }

  QVariantList playlists() const {
    QVariantList list;
    list.reserve(m_lists.size());
    for (const Playlist &p : m_lists)
      list.append(QVariantMap{{QStringLiteral("id"), p.id},
                              {QStringLiteral("name"), p.name},
                              {QStringLiteral("tracks"), p.tracks},
                              {QStringLiteral("count"), p.tracks.size()}});
    return list;
  }

  Q_INVOKABLE bool create(const QString &name) {
    const QString n = name.trimmed();
    if (n.isEmpty() || exists(n))
      return false;
    m_lists.append({QUuid::createUuid().toString(QUuid::WithoutBraces), n, {}});
    save();
    emit changed();
    return true;
  }

  Q_INVOKABLE bool exists(const QString &name) const {
    const QString n = name.trimmed();
    for (const Playlist &p : m_lists)
      if (p.name.compare(n, Qt::CaseInsensitive) == 0)
        return true;
    return false;
  }

  Q_INVOKABLE bool rename(const QString &id, const QString &name) {
    const QString n = name.trimmed();
    if (n.isEmpty())
      return false;
    Playlist *p = find(id);
    if (!p)
      return false;
    for (const Playlist &o : m_lists)
      if (o.id != id && o.name.compare(n, Qt::CaseInsensitive) == 0)
        return false;
    if (p->name == n)
      return true;
    p->name = n;
    save();
    emit changed();
    return true;
  }

  Q_INVOKABLE bool removeList(const QString &id) {
    for (int i = 0; i < m_lists.size(); ++i) {
      if (m_lists[i].id != id)
        continue;
      m_lists.removeAt(i);
      save();
      emit changed();
      return true;
    }
    return false;
  }

  Q_INVOKABLE bool add(const QString &id, const QString &uri) {
    Playlist *p = find(id);
    if (!p || uri.isEmpty() || p->tracks.contains(uri))
      return false;
    p->tracks.append(uri);
    save();
    emit changed();
    return true;
  }

  Q_INVOKABLE bool remove(const QString &id, const QString &uri) {
    Playlist *p = find(id);
    if (!p || !p->tracks.removeOne(uri))
      return false;
    save();
    emit changed();
    return true;
  }

  Q_INVOKABLE bool contains(const QString &id, const QString &uri) const {
    for (const Playlist &p : m_lists)
      if (p.id == id)
        return p.tracks.contains(uri);
    return false;
  }
  Q_INVOKABLE bool isLiked(const QString &uri) const {
    return contains(QStringLiteral("liked"), uri);
  }

  Q_INVOKABLE bool toggleLike(const QString &uri) {
    if (uri.isEmpty())
      return false;
    const QString id = QStringLiteral("liked");
    Playlist *p = find(id);
    if (!p) {
      m_lists.prepend({id, QStringLiteral("Liked songs"), {}});
      p = &m_lists.first();
    }
    const bool nowLiked = !p->tracks.contains(uri);
    if (nowLiked)
      p->tracks.append(uri);
    else
      p->tracks.removeOne(uri);
    save();
    emit changed();
    return nowLiked;
  }
signals:
  void changed();

private:
  struct Playlist {
    QString id;
    QString name;
    QStringList tracks;
  };

  Playlist *find(const QString &id) {
    for (Playlist &p : m_lists)
      if (p.id == id)
        return &p;
    return nullptr;
  }

  void save() const {
    QJsonArray arr;
    for (const Playlist &p : m_lists)
      arr.append(QJsonObject{{"id", p.id},
                             {"name", p.name},
                             {"tracks", QJsonArray::fromStringList(p.tracks)}});
    Store::writePlaylists(arr);
  }

  QList<Playlist> m_lists;
};
