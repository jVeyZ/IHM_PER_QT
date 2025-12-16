#pragma once

#include <QGraphicsObject>
#include <QPointF>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;

class CompassItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit CompassItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setRadius(double r);
    double radius() const { return radius_; }
    void setMinMax(double minR, double maxR) { minRadius_ = minR; maxRadius_ = maxR; }
    // Returns true if the given scene position targets the pivot (center) of the compass
    bool isPointOnPivot(const QPointF &scenePos) const;
    // Returns true if the given scene position targets the movable handle
    bool isPointOnHandle(const QPointF &scenePos) const;
    // Force-start a pivot drag (used as a fallback when the view intercepts events)
    void beginPivotDrag();
    // Force-start a handle drag (used as a fallback when the view intercepts events)
    void beginHandleDrag();
    // Cancel any active dragging state (used when release is missed or focus is lost)
    void cancelDrag();
    // Rotation API
    void beginRotation(const QPointF &scenePos);
    void cancelRotation();

signals:
    void radiusChanged(double newRadius);
    void positionChanged(const QPointF &pos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    double radius_ = 200.0;
    double minRadius_ = 20.0;
    double maxRadius_ = 2000.0;
    bool draggingHandle_ = false;
    bool draggingPivot_ = false;
    bool rotating_ = false;
    qreal handleRadius_ = 12.0;
    qreal pivotRadius_ = 8.0;
    double rotationStartHandleAngle_ = 0.0;
    double rotationStartCompassRotation_ = 0.0;
};
