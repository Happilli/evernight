#pragma once
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QList>
#include <QMouseEvent>
#include <QMutex>
#include <QPointer>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantList>
#include <memory>
#include <vector>

class MaterialList : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QVariantList model READ model WRITE setModel NOTIFY modelChanged)
  Q_PROPERTY(int count READ count NOTIFY modelChanged)
  Q_PROPERTY(
      Variant variant READ variant WRITE setVariant NOTIFY variantChanged)
  Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE
                 setSelectionMode NOTIFY selectionModeChanged)
  Q_PROPERTY(QList<int> checkedIndices READ checkedIndices WRITE
                 setCheckedIndices NOTIFY checkedIndicesChanged)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                 checkedIndicesChanged)
  Q_PROPERTY(bool expandable READ expandable WRITE setExpandable NOTIFY
                 expandableChanged)
  Q_PROPERTY(
      bool accordion READ accordion WRITE setAccordion NOTIFY accordionChanged)
  Q_PROPERTY(int expandedIndex READ expandedIndex WRITE setExpandedIndex NOTIFY
                 expandedIndexChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(
      QFont iconFont READ iconFont WRITE setIconFont NOTIFY iconFontChanged)
  Q_PROPERTY(QColor itemColor READ itemColor WRITE setItemColor NOTIFY
                 itemColorChanged)
  Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor
                 NOTIFY selectedColorChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY
                 textColorChanged)
  Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE
                 setSelectedTextColor NOTIFY selectedTextColorChanged)
  Q_PROPERTY(QColor supportingColor READ supportingColor WRITE
                 setSupportingColor NOTIFY supportingColorChanged)
  Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor NOTIFY
                 iconColorChanged)
  Q_PROPERTY(QColor avatarColor READ avatarColor WRITE setAvatarColor NOTIFY
                 avatarColorChanged)
  Q_PROPERTY(QColor avatarTextColor READ avatarTextColor WRITE
                 setAvatarTextColor NOTIFY avatarTextColorChanged)
  Q_PROPERTY(QColor dividerColor READ dividerColor WRITE setDividerColor NOTIFY
                 dividerColorChanged)

public:
  enum Variant { Standard, Segmented };
  Q_ENUM(Variant)
  enum SelectionMode { NoSelection, Single, Multiple };
  Q_ENUM(SelectionMode)

  explicit MaterialList(QQuickItem *parent = nullptr);

  QVariantList model() const { return m_raw; }
  int count() const { return m_items.size(); }
  Variant variant() const { return m_variant; }
  SelectionMode selectionMode() const { return m_mode; }
  QList<int> checkedIndices() const { return m_checked; }
  int currentIndex() const {
    return m_checked.isEmpty() ? -1 : m_checked.first();
  }
  bool expandable() const { return m_expandable; }
  bool accordion() const { return m_accordion; }
  int expandedIndex() const;
  QFont font() const { return m_font; }
  QFont iconFont() const { return m_iconFont; }
  QColor itemColor() const { return m_itemColor; }
  QColor selectedColor() const { return m_selectedColor; }
  QColor textColor() const { return m_textColor; }
  QColor selectedTextColor() const { return m_selectedTextColor; }
  QColor supportingColor() const { return m_supportingColor; }
  QColor iconColor() const { return m_iconColor; }
  QColor avatarColor() const { return m_avatarColor; }
  QColor avatarTextColor() const { return m_avatarTextColor; }
  QColor dividerColor() const { return m_dividerColor; }

  void setModel(const QVariantList &v);
  void setVariant(Variant v);
  void setSelectionMode(SelectionMode v);
  void setCheckedIndices(const QList<int> &v);
  void setCurrentIndex(int i);
  void setExpandable(bool v);
  void setAccordion(bool v);
  void setExpandedIndex(int i);
  void setFont(const QFont &v);
  void setIconFont(const QFont &v);
  void setItemColor(const QColor &v);
  void setSelectedColor(const QColor &v);
  void setTextColor(const QColor &v);
  void setSelectedTextColor(const QColor &v);
  void setSupportingColor(const QColor &v);
  void setIconColor(const QColor &v);
  void setAvatarColor(const QColor &v);
  void setAvatarTextColor(const QColor &v);
  void setDividerColor(const QColor &v);

  Q_INVOKABLE bool isChecked(int i) const { return m_checked.contains(i); }
  Q_INVOKABLE void setChecked(int i, bool on);
  Q_INVOKABLE void toggleExpanded(int i);
  Q_INVOKABLE void collapseAll();
  Q_INVOKABLE void setDetails(int index, const QVariantList &rows);

signals:
  void trailingClicked(int index, const QVariant &item);
  void expandRequested(int index, const QVariant &item);
  void longPressed(int index, const QVariant &item);
  void modelChanged();
  void variantChanged();
  void selectionModeChanged();
  void checkedIndicesChanged();
  void expandableChanged();
  void accordionChanged();
  void expandedIndexChanged();
  void fontChanged();
  void iconFontChanged();
  void itemColorChanged();
  void selectedColorChanged();
  void textColorChanged();
  void selectedTextColorChanged();
  void supportingColorChanged();
  void iconColorChanged();
  void avatarColorChanged();
  void avatarTextColorChanged();
  void dividerColorChanged();
  void clicked(int index, const QVariant &item);

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void componentComplete() override;
  void geometryChange(const QRectF &n, const QRectF &o) override;
  void itemChange(ItemChange change, const ItemChangeData &data) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private slots:
  void onFlick();

private:
  struct Detail {
    QString label;
    QString value;
  };
  struct DetailRow {
    QString label;
    QStringList lines;
  };
  struct Entry {
    QString overline;
    QString headline;
    QString supporting;
    QString trailingText;
    QString leadingIcon;
    QString trailingIcon;
    QString expandedIcon;
    QString avatarText;
    QString imagePath;
    QString eOver;
    QString eHead;
    QString eSup;
    qreal eAvail = -1.0;
    int eGen = -1;
    bool divider = false;
    bool enabled = true;
    bool open = false;
    bool loading = false;
    std::vector<Detail> details;
    std::vector<DetailRow> dRows;
    qreal dW = -1.0;
    qreal dExtra = 0.0;
    int dGen = -1;
    Kit::Spring ext;
    int lines() const {
      return int(!overline.isEmpty()) + int(!headline.isEmpty()) +
             int(!supporting.isEmpty());
    }
  };
  struct Wanted {
    QMutex m;
    QSet<QString> s;
  };

  QColor resolvedItemColor() const;
  qreal itemGap() const;
  qreal baseHeight(int i) const;
  qreal rowHeight(int i) const;
  qreal leadingWidth(int i) const;
  QList<QMetaObject::Connection> m_ancConns;

  void watchAncestors();
  bool canExpand(int i) const;
  void ensureDetail(int i);
  void retargetExtra(int i);
  void openRow(int i, bool on);
  void relayout();
  void structureChanged();
  void syncHighlight();
  bool selectable(int i) const;
  int rowAt(const QPointF &pt) const;
  bool visibleRows(int &first, int &last) const;
  void activate(int i);
  void attachFlickable();
  void requestThumb(const QString &key, const QString &path, int px, qreal dpr);

  QVariantList m_raw;
  std::vector<Entry> m_items;
  std::vector<qreal> m_tops;
  std::vector<qreal> m_hts;
  Variant m_variant = Standard;
  SelectionMode m_mode = NoSelection;
  QList<int> m_checked;
  bool m_expandable = false;
  bool m_accordion = true;
  bool m_extBusy = false;
  QFont m_font;
  QFont m_iconFont;
  int m_bFirst = -1;
  int m_bLast = -2;
  QTimer m_holdTimer;
  bool m_held = false;
  int m_fontGen = 0;
  QColor m_itemColor;
  QColor m_selectedColor = QColor("#E8DEF8");
  QColor m_textColor = QColor("#1D1B20");
  QColor m_selectedTextColor = QColor("#1D192B");
  QColor m_supportingColor = QColor("#49454F");
  QColor m_iconColor;
  QColor m_avatarColor = QColor("#EADDFF");
  QColor m_avatarTextColor = QColor("#21005D");
  QColor m_dividerColor = QColor("#CAC4D0");
  int m_pressIndex = -1;
  QElapsedTimer m_pressClock;
  int m_flashIndex = -1;
  int m_pressShown = -1;
  Spring m_press;
  Spring m_hlY;
  Spring m_hlH;
  Spring m_hlA;
  QPointer<QQuickItem> m_flick;
  QMetaObject::Connection m_flickConn;
  QSet<QString> m_pending;
  std::shared_ptr<Wanted> m_wanted = std::make_shared<Wanted>();
};
