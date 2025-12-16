#include "compassitem.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPainter>
#include <QPen>

#include <QtMath>
#include <QVariant>

CompassItem::CompassItem(QGraphicsItem *parent)
    : QGraphicsObject(parent) {
    setFlag(ItemIsMovable, true);
    setFlag(ItemSendsGeometryChanges, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
}

QVariant CompassItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged) {
        const QPointF newPos = value.toPointF();
        emit positionChanged(newPos);
    }
    return QGraphicsObject::itemChange(change, value);
}

bool CompassItem::isPointOnPivot(const QPointF &scenePos) const {
    // map from scene to local coords and check distance to pivot
    const QPointF local = mapFromScene(scenePos);
    const QPointF pivot(0.0, 0.0);
    const double pivotDist = std::hypot(local.x() - pivot.x(), local.y() - pivot.y());
    // use a slightly larger tolerance to make grabbing easier
    return pivotDist <= (pivotRadius_ + 8.0);
}

bool CompassItem::isPointOnHandle(const QPointF &scenePos) const {
    const QPointF local = mapFromScene(scenePos);
    const QPointF handle(radius_, 0.0);
    const double handleDist = std::hypot(local.x() - handle.x(), local.y() - handle.y());
    return handleDist <= (handleRadius_ + 8.0);
}

QRectF CompassItem::boundingRect() const {
    const double half = radius_ + handleRadius_ + 8.0;
    return QRectF(-half, -half, half * 2.0, half * 2.0);
}

void CompassItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QPointF center(0.0, 0.0);
    const QPointF handle(radius_, 0.0);

    // Draw fixed leg (pivot)
    painter->setPen(QPen(QColor(45, 109, 163), 2.0));
    painter->setBrush(QBrush(QColor(215, 236, 255)));
    painter->drawEllipse(center, 6.0, 6.0);

    // Draw movable leg
    painter->setPen(QPen(QColor(31, 119, 180), 2.0));
    painter->drawLine(center, handle);
    painter->setBrush(QBrush(QColor(255, 255, 255)));
    painter->drawEllipse(handle, handleRadius_, handleRadius_);

    // Decorative hinge
    painter->setPen(QPen(QColor(100, 100, 100), 1.0));
    painter->drawEllipse(QPointF(0, 0), 3.0, 3.0);
}

void CompassItem::setRadius(double r) {
    const double clamped = std::clamp(r, minRadius_, maxRadius_);
    if (qFuzzyCompare(clamped, radius_)) {
        return;
    }
    prepareGeometryChange();
    radius_ = clamped;
    emit radiusChanged(radius_);
    update();
}

void CompassItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // Clear any stale dragging state (e.g., if a previous drag ended without
    // a proper release event being delivered). This ensures a fresh click in
    // the center won't be misinterpreted as a handle drag.
    if (draggingHandle_ || draggingPivot_) {
        draggingHandle_ = false;
        draggingPivot_ = false;
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
    }

    const QPointF local = event->pos();
    const QPointF handle(radius_, 0.0);
    // Prefer pivot when close to both pivot and handle to avoid accidental radius
    // adjustments right after a radius change.
    const QPointF pivot(0.0, 0.0);
    const double pivotDist = std::hypot(local.x() - pivot.x(), local.y() - pivot.y());
    if (pivotDist <= pivotRadius_ + 8.0) {
        draggingPivot_ = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    const double handleDist = std::hypot(local.x() - handle.x(), local.y() - handle.y());
    if (handleDist <= handleRadius_ + 3.0) {
        draggingHandle_ = true;
        setCursor(Qt::SizeAllCursor);
        event->accept();
        return;
    }

    // Otherwise allow moving the whole compass (also drag pivot if clicked elsewhere)
    QGraphicsObject::mousePressEvent(event);
}

void CompassItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (draggingHandle_) {
        const QPointF local = event->pos();
        const double newRadius = std::hypot(local.x(), local.y());
        setRadius(newRadius);
        event->accept();
        return;
    }
    if (rotating_) {
        const QPointF local = event->pos();
        const double currentAngle = qRadiansToDegrees(std::atan2(-local.y(), local.x()));
        const double delta = currentAngle - rotationStartHandleAngle_;
        // apply delta (subtract because rotation is -angle for handle alignment)
        setRotation(rotationStartCompassRotation_ - delta);
        event->accept();
        return;
    }
    if (draggingPivot_) {
        // Move the pivot to the mouse scene position while keeping the same radius
        const QPointF scenePt = event->scenePos();
        setPos(scenePt);
        event->accept();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void CompassItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (draggingHandle_) {
        draggingHandle_ = false;
        setCursor(Qt::OpenHandCursor);
        // release mouse grab if we acquired it programmatically
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
        event->accept();
        return;
    }
    if (draggingPivot_) {
        draggingPivot_ = false;
        setCursor(Qt::OpenHandCursor);
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
        event->accept();
        return;
    }
    if (rotating_) {
        rotating_ = false;
        setCursor(Qt::OpenHandCursor);
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
        event->accept();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

void CompassItem::beginPivotDrag() {
    draggingPivot_ = true;
    setCursor(Qt::ClosedHandCursor);
    // Ensure this item receives subsequent mouseMove/mouseRelease events
    if (scene() && scene()->mouseGrabberItem() == nullptr) {
        grabMouse();
    }
}

void CompassItem::beginHandleDrag() {
    draggingHandle_ = true;
    setCursor(Qt::SizeAllCursor);
    // Ensure this item receives subsequent mouseMove/mouseRelease events
    if (scene() && scene()->mouseGrabberItem() == nullptr) {
        grabMouse();
    }
}

void CompassItem::cancelDrag() {
    draggingHandle_ = false;
    draggingPivot_ = false;
    setCursor(Qt::OpenHandCursor);
    if (scene() && scene()->mouseGrabberItem() == this) {
        ungrabMouse();
    }
}

void CompassItem::beginRotation(const QPointF &scenePos) {
    // start rotation anchored at the given scene position
    const QPointF local = mapFromScene(scenePos);
    // use same angle convention as toSceneAngle (invert Y)
    rotationStartHandleAngle_ = qRadiansToDegrees(std::atan2(-local.y(), local.x()));
    rotationStartCompassRotation_ = rotation();
    rotating_ = true;
    setCursor(Qt::SizeAllCursor);
    if (scene() && scene()->mouseGrabberItem() == nullptr) {
        grabMouse();
    }
}

void CompassItem::cancelRotation() {
    if (rotating_) {
        rotating_ = false;
        setCursor(Qt::OpenHandCursor);
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
    }
}

void CompassItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    const QPointF local = event->pos();
    const QPointF handle(radius_, 0.0);
    const QPointF pivot(0.0, 0.0);
    const double pivotDist = std::hypot(local.x() - pivot.x(), local.y() - pivot.y());
    if (pivotDist <= pivotRadius_ + 6.0) {
        setCursor(Qt::OpenHandCursor);
        return;
    }
    const double handleDist = std::hypot(local.x() - handle.x(), local.y() - handle.y());
    if (handleDist <= handleRadius_ + 4.0) {
        setCursor(Qt::SizeAllCursor);
        return;
    }
    setCursor(Qt::OpenHandCursor);
}

void CompassItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    setCursor(Qt::OpenHandCursor);
}
