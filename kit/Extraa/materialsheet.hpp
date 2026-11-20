#pragma once
#include "../Core/paintutil.hpp"
#include "../Core/sgitem.hpp"
#include <QColor>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QPointF>
#include <QQmlListProperty>
#include <QRectF>

class MaterialSheet : public SgMaterialItem {
  Q_OBJECT
  QML_ELEMENT
  Q_CLASSINFO("DefaultProperty", "content")
  Q_PROPERTY(QQmlListProperty<QObject> content READ content)
  Q_PROPERTY(QQuickItem *contentItem READ contentItem CONSTANT)
  Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
  Q_PROPERTY(Edge edge READ edge WRITE setEdge NOTIFY edgeChanged)
  Q_PROPERTY(bool modal READ modal WRITE setModal NOTIFY modalChanged)
  Q_PROPERTY(
      bool draggable READ draggable WRITE setDraggable NOTIFY draggableChanged)
  Q_PROPERTY(bool showHandle READ showHandle WRITE setShowHandle NOTIFY
                 showHandleChanged)
  Q_PROPERTY(
      qreal extent READ extentSetting WRITE setExtent NOTIFY extentChanged)
  Q_PROPERTY(qreal maxCrossSize READ maxCrossSize WRITE setMaxCrossSize NOTIFY
                 maxCrossSizeChanged)
  Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(QColor scrimColor READ scrimColor WRITE setScrimColor NOTIFY
                 scrimColorChanged)
  Q_PROPERTY(QColor handleColor READ handleColor WRITE setHandleColor NOTIFY
                 handleColorChanged)

public:
  enum Edge { FromBottom, FromTop, FromLeft, FromRight };
  Q_ENUM(Edge)

  explicit MaterialSheet(QQuickItem *parent = nullptr);

  QQmlListProperty<QObject> content();
  QQuickItem *contentItem() const { return m_panel; }
  bool isOpen() const { return m_open; }
  Edge edge() const { return m_edge; }
  bool modal() const { return m_modal; }
  bool draggable() const { return m_draggable; }
  bool showHandle() const { return m_showHandle; }
  qreal extentSetting() const { return m_extent; }
  qreal maxCrossSize() const { return m_maxCross; }
  qreal radius() const { return m_radius; }
  QColor color() const { return m_color; }
  QColor scrimColor() const { return m_scrimColor; }
  QColor handleColor() const { return m_handleColor; }

  void setOpen(bool v);
  void setEdge(Edge v);
  void setModal(bool v);
  void setDraggable(bool v);
  void setShowHandle(bool v);
  void setExtent(qreal v);
  void setMaxCrossSize(qreal v);
  void setRadius(qreal v);
  void setColor(const QColor &v);
  void setScrimColor(const QColor &v);
  void setHandleColor(const QColor &v);

signals:
  void openChanged();
  void edgeChanged();
  void modalChanged();
  void draggableChanged();
  void showHandleChanged();
  void extentChanged();
  void maxCrossSizeChanged();
  void radiusChanged();
  void colorChanged();
  void scrimColorChanged();
  void handleColorChanged();
  void opened();
  void closed();
  void dismissed();

protected:
  void build(Kit::Mesh &m) override;
  bool advance(qreal dt) override;
  void propertyAssigned() override { applyLayout(); }
  void componentComplete() override;
  void geometryChange(const QRectF &newGeometry,
                      const QRectF &oldGeometry) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseUngrabEvent() override;

private:
  static void appendContent(QQmlListProperty<QObject> *list, QObject *o);
  static qsizetype contentCount(QQmlListProperty<QObject> *list);
  static QObject *contentAt(QQmlListProperty<QObject> *list, qsizetype i);

  bool vertical() const;
  qreal extent() const;
  qreal cornerRadius() const;
  QRectF sheetRect() const;
  QRectF contentRect(const QRectF &sheet) const;
  qreal dragDelta(const QPointF &p) const;
  void applyLayout();
  void retarget(bool immediate = false);
  void dismiss();

  Edge m_edge = FromBottom;
  bool m_open = false;
  bool m_modal = true;
  bool m_draggable = true;
  bool m_showHandle = true;
  qreal m_extent = 0.0;
  qreal m_maxCross = 640.0;
  qreal m_radius = -1.0;
  QColor m_color;
  QColor m_scrimColor;
  QColor m_handleColor;

  QQuickItem *m_panel;
  Spring m_prog;
  bool m_settled = true;
  bool m_drag = false;
  bool m_moved = false;
  bool m_scrimPress = false;
  QPointF m_pressPos;
  qreal m_dragBase = 1.0;
  qreal m_lastDelta = 0.0;
  qreal m_velocity = 0.0;
  QElapsedTimer m_clock;
};
