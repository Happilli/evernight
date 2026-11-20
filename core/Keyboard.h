#pragma once
#include <QGuiApplication>
#include <QInputMethod>
#include <QObject>
#include <QtQml/qqmlregistration.h>

class Keyboard : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  Q_INVOKABLE void hide() { QGuiApplication::inputMethod()->hide(); }
  Q_INVOKABLE void show() { QGuiApplication::inputMethod()->show(); }
};
