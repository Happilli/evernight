#pragma once
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QString>
#include <QStringList>
#include <vector>

class MaterialTabs : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      QStringList labels READ labels WRITE setLabels NOTIFY labelsChanged)
  Q_PROPERTY(QStringList icons READ icons WRITE setIcons NOTIFY iconsChanged)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                 currentIndexChanged)
  Q_PROPERTY(int count READ count NOTIFY countChanged)
  Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(bool fixed READ fixed WRITE setFixed NOTIFY fixedChanged)
  Q_PROPERTY(bool showDivider READ showDivider WRITE setShowDivider NOTIFY
                 showDividerChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(int fontWeight READ fontWeight WRITE setFontWeight NOTIFY
                 fontWeightChanged)
  Q_PROPERTY(
      QFont iconFont READ iconFont WRITE setIconFont NOTIFY iconFontChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor activeColor READ activeColor WRITE setActiveColor NOTIFY
                 activeColorChanged)
  Q_PROPERTY(QColor inactiveColor READ inactiveColor WRITE setInactiveColor
                 NOTIFY inactiveColorChanged)
  Q_PROPERTY(QColor indicatorColor READ indicatorColor WRITE setIndicatorColor
                 NOTIFY indicatorColorChanged)
  Q_PROPERTY(QColor dividerColor READ dividerColor WRITE setDividerColor NOTIFY
                 dividerColorChanged)

public:
  enum Type { Primary, Secondary };
  Q_ENUM(Type)

  explicit MaterialTabs(QQuickItem *parent = nullptr);

  QStringList labels() const { return m_labels; }
  QStringList icons() const { return m_icons; }
  int currentIndex() const { return m_current; }
  int count() const { return int(m_labels.size()); }
  Type type() const { return m_type; }
  bool fixed() const { return m_fixed; }
  bool showDivider() const { return m_showDivider; }
  QFont font() const { return m_font; }
  int fontWeight() const { return m_fontWeight; }
  QFont iconFont() const { return m_iconFont; }
  QColor color() const { return m_color; }
  QColor activeColor() const { return m_activeColor; }
  QColor inactiveColor() const { return m_inactiveColor; }
  QColor indicatorColor() const { return m_indicatorColor; }
  QColor dividerColor() const { return m_dividerColor; }

  void setLabels(const QStringList &v);
  void setIcons(const QStringList &v);
  void setCurrentIndex(int v);
  void setType(Type v);
  void setFixed(bool v);
  void setShowDivider(bool v);
  void setFont(const QFont &v);
  void setFontWeight(int v);
  void setIconFont(const QFont &v);
  void setColor(const QColor &v);
  void setActiveColor(const QColor &v);
  void setInactiveColor(const QColor &v);
  void setIndicatorColor(const QColor &v);
  void setDividerColor(const QColor &v);

signals:
  void labelsChanged();
  void iconsChanged();
  void currentIndexChanged();
  void countChanged();
  void typeChanged();
  void fixedChanged();
  void showDividerChanged();
  void fontChanged();
  void fontWeightChanged();
  void iconFontChanged();
  void colorChanged();
  void activeColorChanged();
  void inactiveColorChanged();
  void indicatorColorChanged();
  void dividerColorChanged();
  void tabClicked(int index);

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void componentComplete() override;
  void geometryChange(const QRectF &newGeometry,
                      const QRectF &oldGeometry) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private:
  QFont labelFont() const;
  bool hasIconAt(int i) const;
  qreal contentWidth(int i) const;
  qreal tabWidth(int i) const;
  qreal tabLeft(int i) const;
  qreal barHeight() const;
  int tabAt(const QPointF &pt) const;
  void syncCount();
  void updateImplicit();
  void retarget(bool immediate = false);
  void setPress(int i);

  QStringList m_labels;
  QStringList m_icons;
  int m_current = 0;
  Type m_type = Primary;
  bool m_fixed = true;
  bool m_showDivider = true;
  QFont m_font;
  int m_fontWeight = 500;
  QFont m_iconFont;
  QColor m_color;
  QColor m_activeColor;
  QColor m_inactiveColor;
  QColor m_indicatorColor;
  QColor m_dividerColor;
  int m_pressIdx = -1;
  bool m_indReady = false;
  qreal m_scaleL = 1.0;
  qreal m_scaleR = 1.0;
  std::vector<qreal> m_textW;
  std::vector<qreal> m_natural;
  std::vector<Spring> m_sel;
  Spring m_indL;
  Spring m_indR;
};
