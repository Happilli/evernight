#pragma once
#include <QColor>
#include <QImage>
#include <QMediaMetaData>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class MediaInfo : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      QMediaMetaData metaData READ metaData WRITE setMetaData NOTIFY changed)
  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString artist READ artist NOTIFY changed)
  Q_PROPERTY(QString album READ album NOTIFY changed)
  Q_PROPERTY(QString coverSource READ coverSource NOTIFY changed)
  Q_PROPERTY(bool hasCover READ hasCover NOTIFY changed)
  Q_PROPERTY(QImage cover READ cover NOTIFY changed)
  Q_PROPERTY(QColor accent READ accent NOTIFY changed)
  Q_PROPERTY(QColor thumbAccent READ thumbAccent NOTIFY thumbAccentChanged)

public:
  using QObject::QObject;

  QColor thumbAccent() const { return m_thumbAccent; }
  void setThumb(const QString &path);
  QMediaMetaData metaData() const { return m_meta; }
  QString title() const { return m_title; }
  QString artist() const { return m_artist; }
  QString album() const { return m_album; }
  QString coverSource() const { return m_coverSource; }
  bool hasCover() const { return !m_coverSource.isEmpty(); }
  QImage cover() const { return m_cover; }
  QColor accent() const { return m_accent; }

  void setMetaData(const QMediaMetaData &meta);

signals:
  void changed();
  void thumbAccentChanged();

private:
  QColor m_thumbAccent;
  QString text(QMediaMetaData::Key key) const;
  static QColor dominant(const QImage &img);

  QMediaMetaData m_meta;
  QString m_title;
  QString m_artist;
  QString m_album;
  QImage m_cover;
  QString m_coverSource;
  QColor m_accent;
  qint64 m_coverKey = 0;
  int m_version = 0;
};
