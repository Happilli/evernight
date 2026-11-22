#pragma once
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QQmlListProperty>
#include <QRectF>
#include <QString>
#include <QStringList>

class MaterialDialog : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_CLASSINFO("DefaultProperty", "content")
  Q_PROPERTY(QQmlListProperty<QObject> content READ content)
  Q_PROPERTY(QQuickItem *contentItem READ contentItem CONSTANT)
  Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  Q_PROPERTY(QString confirmText READ confirmText WRITE setConfirmText NOTIFY
                 confirmTextChanged)
  Q_PROPERTY(QString cancelText READ cancelText WRITE setCancelText NOTIFY
                 cancelTextChanged)
  Q_PROPERTY(bool destructive READ destructive WRITE setDestructive NOTIFY
                 destructiveChanged)
  Q_PROPERTY(bool confirmEnabled READ confirmEnabled WRITE setConfirmEnabled
                 NOTIFY confirmEnabledChanged)
  Q_PROPERTY(bool dismissOnScrim READ dismissOnScrim WRITE setDismissOnScrim
                 NOTIFY dismissOnScrimChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor scrimColor READ scrimColor WRITE setScrimColor NOTIFY
                 scrimColorChanged)
  Q_PROPERTY(QColor titleColor READ titleColor WRITE setTitleColor NOTIFY
                 titleColorChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY
                 textColorChanged)
  Q_PROPERTY(QColor actionColor READ actionColor WRITE setActionColor NOTIFY
                 actionColorChanged)
  Q_PROPERTY(QColor dangerColor READ dangerColor WRITE setDangerColor NOTIFY
                 dangerColorChanged)

public:
  explicit MaterialDialog(QQuickItem *parent = nullptr);

  QQmlListProperty<QObject> content();
  QQuickItem *contentItem() const { return m_panel; }
  bool isOpen() const { return m_open; }
  QString title() const { return m_title; }
  QString text() const { return m_text; }
  QString confirmText() const { return m_confirm; }
  QString cancelText() const { return m_cancel; }
  bool destructive() const { return m_destructive; }
  bool confirmEnabled() const { return m_confirmEnabled; }
  bool dismissOnScrim() const { return m_dismissOnScrim; }
  QFont font() const { return m_font; }
  QColor color() const { return m_color; }
  QColor scrimColor() const { return m_scrimColor; }
  QColor titleColor() const { return m_titleColor; }
  QColor textColor() const { return m_textColor; }
  QColor actionColor() const { return m_actionColor; }
  QColor dangerColor() const { return m_dangerColor; }

  void setOpen(bool v);
  void setTitle(const QString &v);
  void setText(const QString &v);
  void setConfirmText(const QString &v);
  void setCancelText(const QString &v);
  void setDestructive(bool v);
  void setConfirmEnabled(bool v);
  void setDismissOnScrim(bool v);
  void setFont(const QFont &v);
  void setColor(const QColor &v);
  void setScrimColor(const QColor &v);
  void setTitleColor(const QColor &v);
  void setTextColor(const QColor &v);
  void setActionColor(const QColor &v);
  void setDangerColor(const QColor &v);

  Q_INVOKABLE void accept();
  Q_INVOKABLE void reject();

signals:
  void openChanged();
  void titleChanged();
  void textChanged();
  void confirmTextChanged();
  void cancelTextChanged();
  void destructiveChanged();
  void confirmEnabledChanged();
  void dismissOnScrimChanged();
  void fontChanged();
  void colorChanged();
  void scrimColorChanged();
  void titleColorChanged();
  void textColorChanged();
  void actionColorChanged();
  void dangerColorChanged();
  void accepted();
  void rejected();
  void opened();
  void closed();

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void propertyAssigned() override { applyLayout(); }
  void componentComplete() override;
  void geometryChange(const QRectF &n, const QRectF &o) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private:
  enum Zone { None = -1, Cancel = 0, Confirm = 1 };
  struct Layout {
    QRectF card;
    QRectF panel;
    QRectF cancel;
    QRectF confirm;
    qreal titleY = 0.0;
    qreal textY = 0.0;
  };
  static void appendContent(QQmlListProperty<QObject> *l, QObject *o);
  static qsizetype contentCount(QQmlListProperty<QObject> *l);
  static QObject *contentAt(QQmlListProperty<QObject> *l, qsizetype i);

  QFont bodyFont() const;
  QFont actionFont() const;
  qreal contentExtent() const;
  const QStringList &lines(qreal avail) const;
  Layout layout() const;
  int hit(const QPointF &p) const;
  void applyLayout();
  void retarget(bool immediate = false);
  void activate(int zone);

  bool m_open = false;
  QString m_title;
  QString m_text;
  QString m_confirm = QStringLiteral("OK");
  QString m_cancel = QStringLiteral("Cancel");
  bool m_destructive = false;
  bool m_confirmEnabled = true;
  bool m_dismissOnScrim = true;
  QFont m_font;
  QColor m_color = QColor("#ECE6F0");
  QColor m_scrimColor = QColor(Qt::black);
  QColor m_titleColor = QColor("#1D1B20");
  QColor m_textColor = QColor("#49454F");
  QColor m_actionColor = QColor("#6750A4");
  QColor m_dangerColor = QColor("#B3261E");

  QQuickItem *m_panel;
  Spring m_prog;
  bool m_settled = true;
  bool m_scrimPress = false;
  int m_pressZone = None;

  mutable QStringList m_lines;
  mutable QString m_linesKey;
};
