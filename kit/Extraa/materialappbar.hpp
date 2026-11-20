#pragma once
#include "../Core/paintutil.hpp"
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QPointF>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <vector>

class MaterialAppBar : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(
      QString subtitle READ subtitle WRITE setSubtitle NOTIFY subtitleChanged)
  Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(Navigation navigation READ navigation WRITE setNavigation NOTIFY
                 navigationChanged)
  Q_PROPERTY(QUrl navigationIconSource READ navigationIconSource WRITE
                 setNavigationIconSource NOTIFY navigationIconSourceChanged)
  Q_PROPERTY(
      QVariantList actions READ actions WRITE setActions NOTIFY actionsChanged)
  Q_PROPERTY(bool centerTitle READ centerTitle WRITE setCenterTitle NOTIFY
                 centerTitleChanged)
  Q_PROPERTY(
      qreal collapse READ collapse WRITE setCollapse NOTIFY collapseChanged)
  Q_PROPERTY(
      bool scrolled READ scrolled WRITE setScrolled NOTIFY scrolledChanged)
  Q_PROPERTY(qreal expandedHeight READ expandedHeight NOTIFY typeChanged)
  Q_PROPERTY(qreal collapsedHeight READ collapsedHeight CONSTANT)
  Q_PROPERTY(qreal collapseRange READ collapseRange NOTIFY typeChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(
      QFont iconFont READ iconFont WRITE setIconFont NOTIFY iconFontChanged)
  Q_PROPERTY(int fontWeight READ fontWeight WRITE setFontWeight NOTIFY
                 fontWeightChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor scrolledColor READ scrolledColor WRITE setScrolledColor
                 NOTIFY scrolledColorChanged)
  Q_PROPERTY(QColor titleColor READ titleColor WRITE setTitleColor NOTIFY
                 titleColorChanged)
  Q_PROPERTY(QColor subtitleColor READ subtitleColor WRITE setSubtitleColor
                 NOTIFY subtitleColorChanged)
  Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor NOTIFY
                 iconColorChanged)

public:
  enum Type { Small, Medium, Large };
  Q_ENUM(Type)
  enum Navigation { NoNavigation, Back, Menu, Close };
  Q_ENUM(Navigation)

  explicit MaterialAppBar(QQuickItem *parent = nullptr);

  QString title() const { return m_title; }
  QString subtitle() const { return m_subtitle; }
  Type type() const { return m_type; }
  Navigation navigation() const { return m_nav; }
  QUrl navigationIconSource() const { return m_navSource; }
  QVariantList actions() const { return m_raw; }
  bool centerTitle() const { return m_centerTitle; }
  qreal collapse() const { return m_collapse; }
  bool scrolled() const { return m_scrolled; }
  qreal expandedHeight() const;
  qreal collapsedHeight() const;
  qreal collapseRange() const;
  QFont font() const { return m_font; }
  QFont iconFont() const { return m_iconFont; }
  int fontWeight() const { return m_fontWeight; }
  QColor color() const { return m_color; }
  QColor scrolledColor() const { return m_scrolledColor; }
  QColor titleColor() const { return m_titleColor; }
  QColor subtitleColor() const { return m_subtitleColor; }
  QColor iconColor() const { return m_iconColor; }

  void setTitle(const QString &v);
  void setSubtitle(const QString &v);
  void setType(Type v);
  void setNavigation(Navigation v);
  void setNavigationIconSource(const QUrl &v);
  void setActions(const QVariantList &v);
  void setCenterTitle(bool v);
  void setCollapse(qreal v);
  void setScrolled(bool v);
  void setFont(const QFont &v);
  void setIconFont(const QFont &v);
  void setFontWeight(int v);
  void setColor(const QColor &v);
  void setScrolledColor(const QColor &v);
  void setTitleColor(const QColor &v);
  void setSubtitleColor(const QColor &v);
  void setIconColor(const QColor &v);

signals:
  void titleChanged();
  void subtitleChanged();
  void typeChanged();
  void navigationChanged();
  void navigationIconSourceChanged();
  void actionsChanged();
  void centerTitleChanged();
  void collapseChanged();
  void scrolledChanged();
  void fontChanged();
  void iconFontChanged();
  void fontWeightChanged();
  void colorChanged();
  void scrolledColorChanged();
  void titleColorChanged();
  void subtitleColorChanged();
  void iconColorChanged();
  void navigationClicked();
  void actionClicked(int index, const QVariant &item, const QPointF &anchor);

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void componentComplete() override;
  void geometryChange(const QRectF &n, const QRectF &o) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private:
  enum Zone { None = -1, Nav = 0 };

  struct Action {
    QString kind;
    QString glyph;
    QUrl src;
    QString path;
    Kit::IconCache cache;
    bool enabled = true;
  };

  bool flexible() const { return m_type != Small; }
  bool hasNavigation() const;
  qreal expandedPx() const;
  qreal currentHeight() const;
  qreal actionCx(int i) const;
  int hit(const QPointF &pt) const;
  void updateImplicit();
  void retarget(bool immediate = false);
  void setPressZone(int z);
  void activate(int zone);
  void drawNavigation(Kit::Mesh &m, const QPointF &c, const QColor &col);
  void imageLayer(Kit::Mesh &m, int slot, Kit::IconCache &cache, QString &path,
                  const QUrl &src, const QColor &col, const QPointF &c);

  QString m_title;
  QString m_subtitle;
  Type m_type = Small;
  Navigation m_nav = NoNavigation;
  QUrl m_navSource;
  QString m_navPath;
  Kit::IconCache m_navCache;
  QVariantList m_raw;
  std::vector<Action> m_actions;
  bool m_centerTitle = false;
  qreal m_collapse = 0.0;
  bool m_scrolled = false;
  QFont m_font;
  QFont m_iconFont;
  int m_fontWeight = 400;
  QColor m_color = QColor("#FEF7FF");
  QColor m_scrolledColor = QColor("#F3EDF7");
  QColor m_titleColor = QColor("#1D1B20");
  QColor m_subtitleColor = QColor("#49454F");
  QColor m_iconColor = QColor("#49454F");
  int m_pressZone = None;
  Spring m_scr;
};
