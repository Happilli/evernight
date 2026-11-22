#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class ThemeLoader : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QString current READ current WRITE setCurrent NOTIFY changed)
  Q_PROPERTY(QVariantList available READ available NOTIFY changed)
  Q_PROPERTY(QVariantMap dark READ dark NOTIFY changed)
  Q_PROPERTY(QVariantMap light READ light NOTIFY changed)
public:
  explicit ThemeLoader(QObject *parent = nullptr);
  QString current() const { return m_current; }
  QVariantList available() const { return m_available; }
  QVariantMap dark() const { return m_dark; }
  QVariantMap light() const { return m_light; }
  void setCurrent(const QString &id);
signals:
  void changed();

private:
  void scan();
  void reload();
  QString m_current;
  QVariantList m_available;
  QVariantMap m_dark, m_light;
};
