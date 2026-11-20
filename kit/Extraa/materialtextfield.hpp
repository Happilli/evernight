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

class MaterialTextField : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
  Q_PROPERTY(QString placeholderText READ placeholderText WRITE
                 setPlaceholderText NOTIFY placeholderTextChanged)
  Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText
                 NOTIFY supportingTextChanged)
  Q_PROPERTY(Type type READ type WRITE setType NOTIFY typeChanged)
  Q_PROPERTY(
      EchoMode echoMode READ echoMode WRITE setEchoMode NOTIFY echoModeChanged)
  Q_PROPERTY(bool error READ error WRITE setError NOTIFY errorChanged)
  Q_PROPERTY(
      bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
  Q_PROPERTY(bool cursorBlink READ cursorBlink WRITE setCursorBlink NOTIFY
                 cursorBlinkChanged)
  Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength NOTIFY
                 maximumLengthChanged)
  Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition
                 NOTIFY cursorPositionChanged)
  Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(int fontWeight READ fontWeight WRITE setFontWeight NOTIFY
                 fontWeightChanged)
  Q_PROPERTY(QString leadingIconText READ leadingIconText WRITE
                 setLeadingIconText NOTIFY leadingIconTextChanged)
  Q_PROPERTY(QUrl leadingIconSource READ leadingIconSource WRITE
                 setLeadingIconSource NOTIFY leadingIconSourceChanged)
  Q_PROPERTY(QString trailingIconText READ trailingIconText WRITE
                 setTrailingIconText NOTIFY trailingIconTextChanged)
  Q_PROPERTY(QUrl trailingIconSource READ trailingIconSource WRITE
                 setTrailingIconSource NOTIFY trailingIconSourceChanged)
  Q_PROPERTY(
      QFont iconFont READ iconFont WRITE setIconFont NOTIFY iconFontChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor contentColor READ contentColor WRITE setContentColor NOTIFY
                 contentColorChanged)
  Q_PROPERTY(QColor labelColor READ labelColor WRITE setLabelColor NOTIFY
                 labelColorChanged)
  Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY
                 accentColorChanged)
  Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor NOTIFY
                 errorColorChanged)
  Q_PROPERTY(QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY
                 outlineColorChanged)
  Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor
                 NOTIFY selectionColorChanged)

public:
  enum Type { Filled, Outlined };
  Q_ENUM(Type)
  enum EchoMode { Normal, Password };
  Q_ENUM(EchoMode)

  explicit MaterialTextField(QQuickItem *parent = nullptr);
  ~MaterialTextField() override;

  QString text() const { return m_text; }
  QString label() const { return m_label; }
  QString placeholderText() const { return m_placeholder; }
  QString supportingText() const { return m_support; }
  Type type() const { return m_type; }
  EchoMode echoMode() const { return m_echo; }
  bool error() const { return m_error; }
  bool readOnly() const { return m_readOnly; }
  bool cursorBlink() const { return m_cursorBlink; }
  int maximumLength() const { return m_maxLen; }
  int cursorPosition() const;
  QString selectedText() const;
  QFont font() const { return m_font; }
  int fontWeight() const { return m_fontWeight; }
  QString leadingIconText() const { return m_leadText; }
  QUrl leadingIconSource() const { return m_leadSource; }
  QString trailingIconText() const { return m_trailText; }
  QUrl trailingIconSource() const { return m_trailSource; }
  QFont iconFont() const { return m_iconFont; }
  QColor color() const { return m_color; }
  QColor contentColor() const { return m_contentColor; }
  QColor labelColor() const { return m_labelColor; }
  QColor accentColor() const { return m_accentColor; }
  QColor errorColor() const { return m_errorColor; }
  QColor outlineColor() const { return m_outlineColor; }
  QColor selectionColor() const { return m_selectionColor; }

  void setText(const QString &v);
  void setLabel(const QString &v);
  void setPlaceholderText(const QString &v);
  void setSupportingText(const QString &v);
  void setType(Type v);
  void setEchoMode(EchoMode v);
  void setError(bool v);
  void setReadOnly(bool v);
  void setCursorBlink(bool v);
  void setMaximumLength(int v);
  void setCursorPosition(int v);
  void setFont(const QFont &v);
  void setFontWeight(int v);
  void setLeadingIconText(const QString &v);
  void setLeadingIconSource(const QUrl &v);
  void setTrailingIconText(const QString &v);
  void setTrailingIconSource(const QUrl &v);
  void setIconFont(const QFont &v);
  void setColor(const QColor &v);
  void setContentColor(const QColor &v);
  void setLabelColor(const QColor &v);
  void setAccentColor(const QColor &v);
  void setErrorColor(const QColor &v);
  void setOutlineColor(const QColor &v);
  void setSelectionColor(const QColor &v);

  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void clear();
  Q_INVOKABLE void copy();
  Q_INVOKABLE void cut();
  Q_INVOKABLE void paste();
  Q_INVOKABLE void blur();

signals:
  void textChanged();
  void labelChanged();
  void placeholderTextChanged();
  void supportingTextChanged();
  void typeChanged();
  void echoModeChanged();
  void errorChanged();
  void readOnlyChanged();
  void cursorBlinkChanged();
  void maximumLengthChanged();
  void cursorPositionChanged();
  void selectionChanged();
  void fontChanged();
  void fontWeightChanged();
  void leadingIconTextChanged();
  void leadingIconSourceChanged();
  void trailingIconTextChanged();
  void trailingIconSourceChanged();
  void iconFontChanged();
  void colorChanged();
  void contentColorChanged();
  void labelColorChanged();
  void accentColorChanged();
  void errorColorChanged();
  void outlineColorChanged();
  void selectionColorChanged();
  void textEdited();
  void accepted();
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
  void onInputSelection();
  void onInputFocus(bool focused);

private:
  qreal fieldTop() const;
  bool hasLeading() const;
  bool hasTrailing() const;
  bool composing() const;
  qreal padLeft() const;
  qreal padRight() const;
  qreal viewWidth() const;
  qreal textCenter() const;
  QFont textFont() const;
  QColor onSurfaceCol() const;
  QColor variantCol() const;
  QColor accentCol() const;
  QColor errorCol() const;
  QColor outlineCol() const;
  QColor containerCol() const;
  bool inField(const QPointF &pt) const;
  bool overTrailing(const QPointF &pt) const;
  void createInput();
  void applyStyle();
  void layoutInput();
  void focusInput(Qt::FocusReason reason);
  void attachFilter();
  void detachFilter();
  void updateImplicit();
  void retarget(bool immediate = false);
  void iconLayer(Kit::Mesh &m, int slot, Kit::IconCache &cache, QString &path,
                 const QUrl &src, const QString &glyph, const QColor &col,
                 const QPointF &c);

  QQuickItem *m_input = nullptr;
  QPointer<QQuickWindow> m_filtered;

  QString m_text;
  QString m_label;
  QString m_placeholder;
  QString m_support;
  Type m_type = Filled;
  EchoMode m_echo = Normal;
  bool m_error = false;
  bool m_readOnly = false;
  bool m_cursorBlink = false;
  int m_maxLen = -1;
  QFont m_font;
  int m_fontWeight = 400;
  QString m_leadText;
  QUrl m_leadSource;
  QString m_trailText;
  QUrl m_trailSource;
  QFont m_iconFont;
  QColor m_color;
  QColor m_contentColor;
  QColor m_labelColor;
  QColor m_accentColor;
  QColor m_errorColor;
  QColor m_outlineColor;
  QColor m_selectionColor;

  bool m_focused = false;
  bool m_trailPress = false;

  Kit::IconCache m_leadCache;
  Kit::IconCache m_trailCache;
  QString m_leadPath;
  QString m_trailPath;

  Spring m_float;
  Spring m_focus;
};
