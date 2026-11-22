#pragma once
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTimer>

class MaterialSnackbar : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(
      QString message READ message WRITE setMessage NOTIFY messageChanged)
  Q_PROPERTY(QString actionText READ actionText WRITE setActionText NOTIFY
                 actionTextChanged)
  Q_PROPERTY(
      int duration READ duration WRITE setDuration NOTIFY durationChanged)
  Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY
                 bottomMarginChanged)
  Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY
                 textColorChanged)
  Q_PROPERTY(QColor actionColor READ actionColor WRITE setActionColor NOTIFY
                 actionColorChanged)

public:
  explicit MaterialSnackbar(QQuickItem *parent = nullptr);

  bool isOpen() const { return m_open; }
  QString message() const { return m_message; }
  QString actionText() const { return m_action; }
  int duration() const { return m_duration; }
  qreal bottomMargin() const { return m_bottomMargin; }
  qreal radius() const { return m_radius; }
  QFont font() const { return m_font; }
  QColor color() const { return m_color; }
  QColor textColor() const { return m_textColor; }
  QColor actionColor() const { return m_actionColor; }

  void setOpen(bool v);
  void setMessage(const QString &v);
  void setActionText(const QString &v);
  void setDuration(int v);
  void setBottomMargin(qreal v);
  void setRadius(qreal v);
  void setFont(const QFont &v);
  void setColor(const QColor &v);
  void setTextColor(const QColor &v);
  void setActionColor(const QColor &v);

  Q_INVOKABLE void show(const QString &message,
                        const QString &actionText = QString(),
                        int duration = -1);
  Q_INVOKABLE void close() { setOpen(false); }

signals:
  void openChanged();
  void messageChanged();
  void actionTextChanged();
  void durationChanged();
  void bottomMarginChanged();
  void radiusChanged();
  void fontChanged();
  void colorChanged();
  void textColorChanged();
  void actionColorChanged();
  void actionClicked();
  void dismissed();
  void opened();
  void closed();

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void componentComplete() override;
  void geometryChange(const QRectF &n, const QRectF &o) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private:
  struct Layout {
    QRectF card;
    QRectF action;
    qreal textAvail = 0.0;
    bool twoLine = false;
  };

  QFont bodyFont() const;
  QFont actionFont() const;
  const QStringList &lines(qreal avail) const;
  Layout layout() const;
  bool overAction(const QPointF &p) const;
  void retarget(bool immediate = false);
  void restartTimer();

  bool m_open = false;
  QString m_message;
  QString m_action;
  int m_duration = 4000;
  int m_activeDuration = 4000;
  qreal m_bottomMargin = 16.0;
  qreal m_radius = 4.0;
  QFont m_font;
  QColor m_color = QColor("#322F35");
  QColor m_textColor = QColor("#F5EFF7");
  QColor m_actionColor = QColor("#D0BCFF");

  Spring m_prog;
  bool m_settled = true;
  bool m_cardPress = false;
  bool m_actionPress = false;
  QTimer m_timer;

  mutable QStringList m_lines;
  mutable QString m_linesKey;
};
