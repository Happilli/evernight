#include "./materialequalizer.hpp"

namespace {
constexpr qreal kBarW = 4.0;
constexpr qreal kBarGap = 3.0;
constexpr qreal kMinH = 4.0;
} // namespace

MaterialEqualizer::MaterialEqualizer(QQuickItem *parent) : SgItem(parent) {}

void MaterialEqualizer::setLevels(const QList<qreal> &v) {
  if (m_levels == v)
    return;
  m_levels = v;
  m_dirty = true;
  emit levelsChanged();
}

void MaterialEqualizer::itemChange(ItemChange change,
                                   const ItemChangeData &data) {
  SgItem::itemChange(change, data);
  if (change != ItemSceneChange)
    return;
  if (m_conn) {
    disconnect(m_conn);
    m_conn = {};
  }
  if (data.window)
    m_conn = connect(data.window, &QQuickWindow::afterAnimating, this, [this] {
      if (!m_dirty)
        return;
      m_dirty = false;
      update();
    });
}
void MaterialEqualizer::setColor(const QColor &v) {
  assign(m_color, v, &MaterialEqualizer::colorChanged);
}

void MaterialEqualizer::build(Kit::Mesh &m) {
  if (width() <= 0.0 || height() <= 0.0)
    return;
  const int nb = m_levels.isEmpty() ? 3 : int(m_levels.size());
  const qreal maxH = width() * 0.45;
  const qreal total = nb * kBarW + (nb - 1) * kBarGap;
  const qreal baseY = height() / 2.0 + maxH / 2.0;
  qreal bx = width() / 2.0 - total / 2.0;
  for (int b = 0; b < nb; ++b) {
    const qreal lv =
        b < m_levels.size() ? std::clamp(m_levels[b], 0.0, 1.0) : 0.0;
    const qreal bh = kMinH + (maxH - kMinH) * lv;
    m.fillRoundedRect(QRectF(m.snap(bx), baseY - bh, kBarW, bh), kBarW / 2.0,
                      kBarW / 2.0, kBarW / 2.0, kBarW / 2.0, m_color);
    bx += kBarW + kBarGap;
  }
}
