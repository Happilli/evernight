#include "./MediaInfo.h"
#include "./CoverProvider.h"
#include <QImage>

QString MediaInfo::text(QMediaMetaData::Key key) const {
  return m_meta.stringValue(key).trimmed();
}

QColor MediaInfo::dominant(const QImage &img) {
  const QImage s =
      img.scaled(24, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
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

void MediaInfo::setMetaData(const QMediaMetaData &meta) {
  m_meta = meta;

  m_title = text(QMediaMetaData::Title);
  m_album = text(QMediaMetaData::AlbumTitle);
  m_artist = text(QMediaMetaData::ContributingArtist);
  if (m_artist.isEmpty())
    m_artist = text(QMediaMetaData::AlbumArtist);
  if (m_artist.isEmpty())
    m_artist = text(QMediaMetaData::Author);

  QImage cover = m_meta.value(QMediaMetaData::CoverArtImage).value<QImage>();
  m_cover = cover;

  if (cover.isNull()) {
    CoverProvider::set(QImage());
    m_coverKey = 0;
    m_coverSource.clear();
    m_accent = QColor();
  } else if (cover.cacheKey() != m_coverKey) {
    CoverProvider::set(cover);
    m_coverKey = cover.cacheKey();
    m_coverSource =
        QStringLiteral("image://cover/") + QString::number(++m_version);
    m_accent = dominant(cover);
  }

  emit changed();
}
