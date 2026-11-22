#pragma once
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QList>
#include <QMetaObject>
#include <QQuickWindow>

class MaterialEqualizer : public SgItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      QList<qreal> levels READ levels WRITE setLevels NOTIFY levelsChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
  explicit MaterialEqualizer(QQuickItem *parent = nullptr);
  QList<qreal> levels() const { return m_levels; }
  QColor color() const { return m_color; }
  void setLevels(const QList<qreal> &v);
  void setColor(const QColor &v);

signals:
  void levelsChanged();
  void colorChanged();

protected:
  void itemChange(ItemChange change, const ItemChangeData &data) override;
  void build(Kit::Mesh &m) override;

private:
  QMetaObject::Connection m_conn;
  bool m_dirty = false;
  QList<qreal> m_levels;
  QColor m_color = QColor("#21005D");
};
