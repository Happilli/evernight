#include "./core/CoverProvider.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[]) {
// #ifdef Q_OS_ANDROID
//   qputenv("QT_MEDIA_BACKEND", "android");
// #endif
  QGuiApplication app(argc, argv);
  app.setOrganizationName("safalski");
  app.setApplicationName("Evernight");
  QQmlApplicationEngine engine;
  engine.addImageProvider("cover", new CoverProvider);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.loadFromModule("Evernight", "Main");
  return app.exec();
}
