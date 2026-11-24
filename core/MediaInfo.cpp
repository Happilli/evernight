#include "./MediaInfo.h"
#include "./CoverProvider.h"
#include <QImage>

QString MediaInfo::text(QMediaMetaData::Key key) const {
  return m_meta.stringValue(key).trimmed();
}

QColor MediaInfo::dominant(const QImage &img) {
  const QImage s =
      img.scaled(96, 96, Qt::IgnoreAspectRatio, Qt::FastTransformation)
          .scaled(24, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
          .convertToFormat(QImage::Format_RGB32);
  double r = 0, g = 0, b = 0, wsum = 0;
  for (int y = 0; y < s.height(); ++y) {
    for (int x = 0; x < s.width(); ++x) {
      const QColor c = s.pixelColor(x, y);
      const double w = 0.05 + c.hsvSaturationF() * c.valueF();
      r += c.redF() * w;
      g += c.greenF() * w;
      b += c.blueF() * w;
      wsum += w;
    }
  }
  if (wsum <= 0)
    return QColor();
  return QColor::fromRgbF(r / wsum, g / wsum, b / wsum);
}

void MediaInfo::setThumb(const QString &path) {
  const QImage img(path);
  m_thumbAccent = img.isNull() ? QColor() : dominant(img);
  emit thumbAccentChanged();
}
void MediaInfo::setMetaData(const QMediaMetaData &meta) {
  m_meta = meta;
  const QString title = text(QMediaMetaData::Title);
  const QString album = text(QMediaMetaData::AlbumTitle);
  QString artist = text(QMediaMetaData::ContributingArtist);
  if (artist.isEmpty())
    artist = text(QMediaMetaData::AlbumArtist);
  if (artist.isEmpty())
    artist = text(QMediaMetaData::Author);

  bool dirty = title != m_title || album != m_album || artist != m_artist;
  m_title = title;
  m_album = album;
  m_artist = artist;

  const QImage cover =
      m_meta.value(QMediaMetaData::CoverArtImage).value<QImage>();
  if (cover.isNull()) {
    if (!m_cover.isNull()) {
      CoverProvider::set(QImage());
      m_cover = QImage();
      m_coverKey = 0;
      m_coverSource.clear();
      m_accent = QColor();
      dirty = true;
    }
  } else if (cover.cacheKey() != m_coverKey) {
    m_cover = cover;
    CoverProvider::set(cover);
    m_coverKey = cover.cacheKey();
    m_coverSource =
        QStringLiteral("image://cover/") + QString::number(++m_version);
    m_accent = dominant(cover);
    dirty = true;
  }

  if (dirty)
    emit changed();
}
