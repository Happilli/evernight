#pragma once
#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QHash>
#include <QPropertyAnimation>
#include <QQuickItem>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <vector>

class MorphNode;

class MorphText : public QQuickItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(
      int duration READ duration WRITE setDuration NOTIFY durationChanged)
  Q_PROPERTY(
      qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
  Q_PROPERTY(
      qreal stiffness READ stiffness WRITE setStiffness NOTIFY springChanged)
  Q_PROPERTY(qreal damping READ damping WRITE setDamping NOTIFY springChanged)

public:
  explicit MorphText(QQuickItem *parent = nullptr);
  QString text() const { return m_text; }
  QColor color() const { return m_color; }
  QFont font() const { return m_font; }
  int duration() const { return m_duration; }
  qreal progress() const { return m_progress; }
  qreal stiffness() const { return m_k; }
  qreal damping() const { return m_c; }
  void setStiffness(qreal v) {
    m_k = v;
    emit springChanged();
  }
  void setDamping(qreal v) {
    m_c = v;
    emit springChanged();
  }
  void setText(const QString &v);
  void setColor(const QColor &v);
  void setFont(const QFont &v);
  void setDuration(int v);
  void setProgress(qreal v);

signals:
  void springChanged();
  void textChanged();
  void colorChanged();
  void fontChanged();
  void durationChanged();
  void progressChanged();

protected:
  QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *d) override;
  void geometryChange(const QRectF &n, const QRectF &o) override;

private:
  struct G {
    QChar ch;
    qreal x;
  };
  struct Item {
    QChar ch;
    qreal fromX;
    qreal toX;
    int kind;
  };
  void rebuild(bool animate);
  void updateImplicit();
  std::vector<G> layout(const QString &s) const;
  void buildAtlas(MorphNode *n);

  QString m_text;
  QColor m_color = Qt::white;
  QFont m_font;
  int m_duration = 380;
  qreal m_progress = 1.0;
  std::vector<Item> m_items;
  QPropertyAnimation m_anim;
  bool m_atlasDirty = true;
  QHash<ushort, int> m_cellPx;
  int m_atlasW = 1;
  int m_atlasH = 1;
  int m_cellH = 1;
  qreal m_dpr = 1.0;
  qreal m_k = 380.0;
  qreal m_c = 22.0;
  qreal m_vel = 0.0;
  bool m_run = false;
  QElapsedTimer m_clock;
};
