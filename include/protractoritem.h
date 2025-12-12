#pragma once

#include <QGraphicsObject>
#include <QtSvg/QSvgRenderer>
#include <memory>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;

class QSvgRenderer;

class ProtractorItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit ProtractorItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setRadius(double radius);
    double radius() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    double radius_ = 200.0;
    double minRadius_ = 120.0;
    double maxRadius_ = 400.0;
    double svgAspectRatio_ = 1.0;
    std::unique_ptr<QSvgRenderer> svgRenderer_;
    
    // Rotation state
    bool rotating_ = false;
    double startRotation_ = 0.0;
    double startAngle_ = 0.0;
    QPointF rotationCenterScene_;
};
