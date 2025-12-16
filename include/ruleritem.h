#pragma once

#include <QGraphicsObject>

#include <QCursor>
#include <QPointF>

class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneHoverEvent;

class RulerItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit RulerItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setLength(double length);
    double length() const;
    void resetState();
    // Cancel any active interaction (rotation/resizing)
    void cancelInteraction();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

 private:
    double length_ = 2340.0;
    qreal startRotation_ = 0.0;
    // Last drawn pixmap rect in local coordinates (used to avoid invisible SVG margins)
    QRectF drawnRectLocal_ = QRectF();
    double startAngle_ = 0.0;
    QPointF startScenePos_;
    QPointF rotationCenterScene_;
    QPointF rotationStartVector_;
    QPointF lastPointerScenePos_;
    bool rotating_ = false;
    bool resizing_ = false;
    bool moving_ = false;
    QPointF anchorScenePos_;
    QPointF rotationPivotScene_;
    QPointF rotationPivotLocal_;
    enum class ResizeEdge { None, Left, Right } resizeEdge_ = ResizeEdge::None;
};
