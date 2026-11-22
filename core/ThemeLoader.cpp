#include "./ThemeLoader.h"
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

namespace {
struct Parsed {
  QString name;
  QVariantMap common, dark, light;
  bool ok = false;
};
const QString kBuiltinDir =
    QStringLiteral(":/qt/qml/Evernight/theme/variation");

Parsed parseIni(const QString &path) {
  Parsed p;
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return p;
  QString section;
  while (!f.atEnd()) {
    const QString line = QString::fromUtf8(f.readLine()).trimmed();
    if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
      continue;
    if (line.startsWith('[') && line.endsWith(']')) {
      section = line.mid(1, line.size() - 2).trimmed().toLower();
      continue;
    }
    const int eq = line.indexOf('=');
    if (eq <= 0)
      continue;
    const QString key = line.left(eq).trimmed();
    const QString val = line.mid(eq + 1).trimmed();
    if (section == QLatin1String("theme") && key == QLatin1String("name"))
      p.name = val;
    else if (section == QLatin1String("common"))
      p.common.insert(key, val);
    else if (section == QLatin1String("dark"))
      p.dark.insert(key, val);
    else if (section == QLatin1String("light"))
      p.light.insert(key, val);
  }
  p.ok = true;
  return p;
}

QString userDir() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QStringLiteral("/themes");
}

QString pathFor(const QString &id) {
  const QString user = userDir() + '/' + id + QStringLiteral(".ini");
  if (QFile::exists(user))
    return user;
  return kBuiltinDir + '/' + id + QStringLiteral(".ini");
}

QVariantMap merged(const QVariantMap &common, const QVariantMap &mode) {
  QVariantMap m = common;
  m.insert(mode);
  return m;
}
} // namespace

ThemeLoader::ThemeLoader(QObject *parent) : QObject(parent) {
  m_current =
      QSettings().value("themeName", QStringLiteral("monochrome")).toString();
  scan();
  reload();
}

void ThemeLoader::scan() {
  m_available.clear();
  QStringList ids;
  for (const QString &dir : {kBuiltinDir, userDir()})
    for (const QString &f : QDir(dir).entryList({"*.ini"}, QDir::Files)) {
      const QString id = QFileInfo(f).completeBaseName();
      if (!ids.contains(id))
        ids << id;
    }
  ids.sort();
  ids.removeAll("monochrome");
  ids.prepend("monochrome");
  for (const QString &id : ids) {
    const Parsed p = parseIni(pathFor(id));
    if (!p.ok)
      continue;
    m_available.append(
        QVariantMap{{"id", id}, {"name", p.name.isEmpty() ? id : p.name}});
  }
}

void ThemeLoader::reload() {
  const Parsed base = parseIni(pathFor("monochrome"));
  m_dark = merged(base.common, base.dark);
  m_light = merged(base.common, base.light);
  if (m_current != QLatin1String("monochrome")) {
    const Parsed sel = parseIni(pathFor(m_current));
    if (sel.ok) {
      m_dark.insert(merged(sel.common, sel.dark));
      m_light.insert(merged(sel.common, sel.light));
    }
  }
  emit changed();
}

void ThemeLoader::setCurrent(const QString &id) {
  if (id == m_current)
    return;
  m_current = id;
  QSettings().setValue("themeName", id);
  reload();
}
