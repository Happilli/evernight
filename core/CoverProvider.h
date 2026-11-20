#pragma once
#include <QImage>
#include <QQuickImageProvider>

class CoverProvider : public QQuickImageProvider {
public:
  CoverProvider();
  QImage requestImage(const QString &id, QSize *size,
                      const QSize &requested) override;
  static void set(const QImage &image);
};
