#pragma once
#include "../Core/paintutil.hpp"
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QMouseEvent>
#include <QPointer>
#include <QQuickWindow>
#include <QString>
#include <QUrl>

class MaterialSearchBox : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  Q_PROPERTY(QString placeholderText READ placeholderText WRITE
                 setPlaceholderText NOTIFY placeholderTextChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(int fontWeight READ fontWeight WRITE setFontWeight NOTIFY
                 fontWeightChanged)
  Q_PROPERTY(
      QFont iconFont READ iconFont WRITE setIconFont NOTIFY iconFontChanged)
  Q_PROPERTY(QString leadingIconText READ leadingIconText WRITE
                 setLeadingIconText NOTIFY leadingIconTextChanged)
  Q_PROPERTY(QString trailingIconText READ trailingIconText WRITE
                 setTrailingIconText NOTIFY trailingIconTextChanged)
  Q_PROPERTY(QUrl leadingIconSource READ leadingIconSource WRITE
                 setLeadingIconSource NOTIFY leadingIconSourceChanged)
  Q_PROPERTY(QUrl trailingIconSource READ trailingIconSource WRITE
                 setTrailingIconSource NOTIFY trailingIconSourceChanged)
  Q_PROPERTY(QUrl clearIconSource READ clearIconSource WRITE setClearIconSource
                 NOTIFY clearIconSourceChanged)
  Q_PROPERTY(bool showSearchIcon READ showSearchIcon WRITE setShowSearchIcon
                 NOTIFY showSearchIconChanged)
  Q_PROPERTY(
      bool clearable READ clearable WRITE setClearable NOTIFY clearableChanged)
  Q_PROPERTY(
      bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
  Q_PROPERTY(
      int maxLength READ maxLength WRITE setMaxLength NOTIFY maxLengthChanged)
  Q_PROPERTY(
      int cursorPosition READ cursorPosition NOTIFY cursorPositionChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY
                 textColorChanged)
  Q_PROPERTY(QColor placeholderColor READ placeholderColor WRITE
                 setPlaceholderColor NOTIFY placeholderColorChanged)
  Q_PROPERTY(QColor iconColor READ iconColor WRITE setIconColor NOTIFY
                 iconColorChanged)
  Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY
                 borderColorChanged)
  Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY
                 borderWidthChanged)
  Q_PROPERTY(QColor cursorColor READ cursorColor WRITE setCursorColor NOTIFY
                 cursorColorChanged)
  Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor
                 NOTIFY selectionColorChanged)

public:
  explicit MaterialSearchBox(QQuickItem *parent = nullptr);
  ~MaterialSearchBox() override;

  QString text() const { return m_text; }
  QString placeholderText() const { return m_placeholder; }
  QFont font() const { return m_font; }
  int fontWeight() const { return m_fontWeight; }
  QFont iconFont() const { return m_iconFont; }
  QString leadingIconText() const { return m_leadingIconText; }
  QString trailingIconText() const { return m_trailingIconText; }
  QUrl leadingIconSource() const { return m_leadingIconSource; }
  QUrl trailingIconSource() const { return m_trailingIconSource; }
  QUrl clearIconSource() const { return m_clearIconSource; }
  bool showSearchIcon() const { return m_showSearchIcon; }
  bool clearable() const { return m_clearable; }
  bool readOnly() const { return m_readOnly; }
  int maxLength() const { return m_maxLength; }
  int cursorPosition() const;
  QColor color() const { return m_color; }
  QColor textColor() const { return m_textColor; }
  QColor placeholderColor() const { return m_placeholderColor; }
  QColor iconColor() const { return m_iconColor; }
  QColor borderColor() const { return m_borderColor; }
  qreal borderWidth() const { return m_borderWidth; }
  QColor cursorColor() const { return m_cursorColor; }
  QColor selectionColor() const { return m_selectionColor; }

  void setText(const QString &v);
  void setPlaceholderText(const QString &v);
  void setFont(const QFont &v);
  void setFontWeight(int v);
  void setIconFont(const QFont &v);
  void setLeadingIconText(const QString &v);
  void setTrailingIconText(const QString &v);
  void setLeadingIconSource(const QUrl &v);
  void setTrailingIconSource(const QUrl &v);
  void setClearIconSource(const QUrl &v);
  void setShowSearchIcon(bool v);
  void setClearable(bool v);
  void setReadOnly(bool v);
  void setMaxLength(int v);
  void setColor(const QColor &v);
  void setTextColor(const QColor &v);
  void setPlaceholderColor(const QColor &v);
  void setIconColor(const QColor &v);
  void setBorderColor(const QColor &v);
  void setBorderWidth(qreal v);
  void setCursorColor(const QColor &v);
  void setSelectionColor(const QColor &v);

  Q_INVOKABLE void clear();
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void blur();

signals:
  void textChanged();
  void placeholderTextChanged();
  void fontChanged();
  void fontWeightChanged();
  void iconFontChanged();
  void leadingIconTextChanged();
  void trailingIconTextChanged();
  void leadingIconSourceChanged();
  void trailingIconSourceChanged();
  void clearIconSourceChanged();
  void showSearchIconChanged();
  void clearableChanged();
  void readOnlyChanged();
  void maxLengthChanged();
  void cursorPositionChanged();
  void colorChanged();
  void textColorChanged();
  void placeholderColorChanged();
  void iconColorChanged();
  void borderColorChanged();
  void borderWidthChanged();
  void cursorColorChanged();
  void selectionColorChanged();
  void textEdited();
  void accepted();
  void cleared();
  void leadingClicked();
  void trailingClicked();

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void componentComplete() override;
  void propertyAssigned() override;
  void geometryChange(const QRectF &n, const QRectF &o) override;
  void itemChange(ItemChange change, const ItemChangeData &data) override;
  bool eventFilter(QObject *watched, QEvent *event) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private slots:
  void onInputText();
  void onInputEdited();
  void onInputAccepted();
  void onInputCursor();
  void onInputFocus(bool focused);

private:
  enum Zone { None = -1, Leading = 0, Trailing = 1, Clear = 2, Text = 3 };

  struct IconSlot {
    Kit::IconCache cache;
    QString path;
  };

  QFont labelFont() const;
  bool leadingShown() const;
  bool trailingShown() const;
  bool clearVisible() const;
  bool composing() const;
  qreal leadingCx() const;
  qreal trailingCx() const;
  qreal clearCx() const;
  qreal textLeft() const;
  qreal textRight() const;
  int zoneAt(const QPointF &pt) const;
  void createInput();
  void applyStyle();
  void layoutInput();
  void focusInput(Qt::FocusReason reason);
  void attachFilter();
  void detachFilter();
  void retarget(bool immediate = false);
  void iconLayer(Kit::Mesh &m, int slot, IconSlot &s, const QUrl &src, qreal cx,
                 qreal cy, qreal size);

  QQuickItem *m_input = nullptr;
  QPointer<QQuickWindow> m_filtered;

  QString m_text;
  QString m_placeholder;
  QFont m_font;
  int m_fontWeight = 400;
  QFont m_iconFont;
  QString m_leadingIconText;
  QString m_trailingIconText;
  QUrl m_leadingIconSource;
  QUrl m_trailingIconSource;
  QUrl m_clearIconSource;
  IconSlot m_leading;
  IconSlot m_trailing;
  IconSlot m_clearSlot;
  bool m_showSearchIcon = true;
  bool m_clearable = true;
  bool m_readOnly = false;
  int m_maxLength = 0;

  bool m_focused = false;
  int m_pressZone = None;

  QColor m_color = QColor("#ECE6F0");
  QColor m_textColor = QColor("#1D1B20");
  QColor m_placeholderColor = QColor("#49454F");
  QColor m_iconColor = QColor("#49454F");
  QColor m_borderColor = QColor("#79747E");
  qreal m_borderWidth = 0.0;
  QColor m_cursorColor = QColor("#6750A4");
  QColor m_selectionColor = QColor(103, 80, 164, 80);

  Spring m_foc;
  Spring m_clr;
};
