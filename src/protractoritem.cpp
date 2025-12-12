#include "protractoritem.h"

#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QSvgRenderer>

#include <QtMath>

#include <algorithm>
#include <cmath>
#include <memory>

ProtractorItem::ProtractorItem(QGraphicsItem *parent)
    : QGraphicsObject(parent) {
    setFlag(ItemIsMovable, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    svgRenderer_ = std::make_unique<QSvgRenderer>(QStringLiteral(":/resources/images/transportador.svg"));
    if (svgRenderer_ && svgRenderer_->isValid()) {
        const QSizeF defaultSize = svgRenderer_->defaultSize();
        if (!defaultSize.isEmpty() && defaultSize.width() > 0.0) {
            svgAspectRatio_ = defaultSize.height() / defaultSize.width();
        }
    }
}

QRectF ProtractorItem::boundingRect() const {
    const double width = radius_ * 2.0;
    const double aspect = svgAspectRatio_ > 0.0 ? svgAspectRatio_ : 1.0;
    const double height = width * aspect;
    // Center the bounding rect around the origin
    return QRectF(-radius_, -height / 2.0, width, height);
}

void ProtractorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    if (!svgRenderer_ || !svgRenderer_->isValid()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    const QRectF targetRect = boundingRect();
    svgRenderer_->render(painter, targetRect);
}

void ProtractorItem::setRadius(double radius) {
    const double clamped = std::clamp(radius, minRadius_, maxRadius_);
    if (qFuzzyCompare(clamped, radius_)) {
        return;
    }
    prepareGeometryChange();
    radius_ = clamped;
}

double ProtractorItem::radius() const {
    return radius_;
}

void ProtractorItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    const QRectF bounds = boundingRect();
    const QPointF localPos = event->pos();
    
    // Check if near the edges for rotation (top/bottom edge)
    const bool nearTop = localPos.y() < bounds.top() + 20.0;
    const bool nearBottom = localPos.y() > bounds.bottom() - 20.0;
    
    if (nearTop || nearBottom) {
        rotating_ = true;
        startRotation_ = rotation();
        rotationCenterScene_ = mapToScene(QPointF(0, 0));
        const QPointF vec = event->scenePos() - rotationCenterScene_;
        startAngle_ = std::atan2(vec.y(), vec.x());
        setCursor(Qt::SizeAllCursor);
        event->accept();
        return;
    }

    // Default: dragging
    QGraphicsObject::mousePressEvent(event);
}

void ProtractorItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (rotating_) {
        const QPointF currentVector = event->scenePos() - rotationCenterScene_;
        const double currentAngle = std::atan2(currentVector.y(), currentVector.x());
        const double delta = currentAngle - startAngle_;
        setRotation(startRotation_ + qRadiansToDegrees(delta));
        event->accept();
        return;
    }

    QGraphicsObject::mouseMoveEvent(event);
}

void ProtractorItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (rotating_) {
        rotating_ = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void ProtractorItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    const QRectF bounds = boundingRect();
    const QPointF localPos = event->pos();
    
    // Show rotation cursor near top/bottom edges
    if (localPos.y() < bounds.top() + 20.0 || localPos.y() > bounds.bottom() - 20.0) {
        setCursor(Qt::SizeAllCursor);
        return;
    }
    
    setCursor(Qt::OpenHandCursor);
}

void ProtractorItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    setCursor(Qt::OpenHandCursor);
}
