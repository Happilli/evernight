#include "./CoverProvider.h"
#include <QMutex>
#include <QMutexLocker>

namespace {
QMutex mutex;
QImage current;
} // namespace

CoverProvider::CoverProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage CoverProvider::requestImage(const QString &, QSize *size,
                                   const QSize &requested) {
  QImage image;
  {
    QMutexLocker lock(&mutex);
    image = current;
  }
  if (size)
    *size = image.size();
  if (!image.isNull() && requested.isValid())
    image =
        image.scaled(requested, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  return image;
}

void CoverProvider::set(const QImage &image) {
  QMutexLocker lock(&mutex);
  current = image;
}
