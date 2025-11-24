#include "ruleritem.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>

#include <QtMath>
#include <algorithm>

RulerItem::RulerItem(QGraphicsItem *parent)
    : QGraphicsObject(parent) {
    setFlag(ItemIsMovable, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
}

QRectF RulerItem::boundingRect() const {
    return QRectF(-length_ / 2.0, -18.0, length_, 36.0);
}

void RulerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = boundingRect();
    painter->setBrush(QColor(215, 236, 255, 200));
    painter->setPen(QPen(QColor(45, 109, 163), 2.0));
    painter->drawRoundedRect(rect, 6.0, 6.0);

    painter->setPen(QPen(QColor(31, 119, 180), 1.5));

    const double step = length_ / 10.0;
    for (int i = 0; i <= 10; ++i) {
        const double x = rect.left() + step * i;
        const double top = (i % 2 == 0) ? rect.top() + 4.0 : rect.top() + 10.0;
        painter->drawLine(QPointF(x, top), QPointF(x, rect.bottom() - 4.0));
    }
}

void RulerItem::setLength(double length) {
    prepareGeometryChange();
    length_ = std::max(length, 40.0);
}

double RulerItem::length() const {
    return length_;
}

void RulerItem::resetState() {
    setLength(260.0);
    setRotation(0.0);
    setPos(QPointF(0.0, 0.0));
}

void RulerItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    const QRectF bounds = boundingRect();
    const qreal edgeThreshold = 14.0;
    const QPointF localPos = event->pos();
    const bool nearLeft = std::abs(localPos.x() - bounds.left()) < edgeThreshold;
    const bool nearRight = std::abs(localPos.x() - bounds.right()) < edgeThreshold;
    const bool nearTop = localPos.y() < bounds.top() + 10.0;
    const bool nearBottom = localPos.y() > bounds.bottom() - 10.0;
    const double edgeRegion = length_ * 0.2;
    const bool nearLeftEdge = std::abs(localPos.x() - bounds.left()) < edgeRegion;
    const bool nearRightEdge = std::abs(localPos.x() - bounds.right()) < edgeRegion;

    startScenePos_ = event->scenePos();
    startRotation_ = rotation();
    rotationCenterScene_ = mapToScene(QPointF(0, 0));
    rotationStartVector_ = event->scenePos() - rotationCenterScene_;
    startAngle_ = std::atan2(rotationStartVector_.y(), rotationStartVector_.x());
    lastPointerScenePos_ = event->scenePos();

    if (nearTop || nearBottom) {
        rotating_ = true;
        resizeEdge_ = ResizeEdge::None;
        setCursor(Qt::SizeAllCursor);
        rotationPivotScene_ = rotationCenterScene_;
        rotationPivotLocal_ = QPointF(0.0, 0.0);
        if (nearLeftEdge || nearRightEdge) {
            const double pivotX = nearLeftEdge ? bounds.right() : bounds.left();
            rotationPivotLocal_ = QPointF(pivotX, 0.0);
            rotationPivotScene_ = mapToScene(rotationPivotLocal_);
        }
        event->accept();
        return;
    }
    if (nearLeft || nearRight) {
        resizing_ = true;
        resizeEdge_ = nearLeft ? ResizeEdge::Left : ResizeEdge::Right;
        const qreal anchorLocalX = (resizeEdge_ == ResizeEdge::Left) ? bounds.right() : bounds.left();
        anchorScenePos_ = mapToScene(QPointF(anchorLocalX, 0));
        setCursor(Qt::SizeHorCursor);
        event->accept();
        return;
    }

    resizeEdge_ = ResizeEdge::None;
    QGraphicsObject::mousePressEvent(event);
}

void RulerItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (resizing_) {
        const QPointF handleDirScene = mapToScene(QPointF(resizeEdge_ == ResizeEdge::Right ? 1.0 : -1.0, 0.0)) -
                                        mapToScene(QPointF(0.0, 0.0));
        const double handleNorm = std::hypot(handleDirScene.x(), handleDirScene.y());
        QPointF axis(1.0, 0.0);
        if (handleNorm > 0.0) {
            axis = handleDirScene / handleNorm;
        }
        const QPointF movement = event->scenePos() - lastPointerScenePos_;
        const double projection = QPointF::dotProduct(movement, axis);
        setLength(length_ + projection);
        const QRectF bounds = boundingRect();
        const qreal anchorLocalX = (resizeEdge_ == ResizeEdge::Left) ? bounds.right() : bounds.left();
        const QPointF currentAnchorScene = mapToScene(QPointF(anchorLocalX, 0));
        const QPointF shift = anchorScenePos_ - currentAnchorScene;
        setPos(pos() + shift);
        lastPointerScenePos_ = event->scenePos();
        event->accept();
        return;
    }
    if (rotating_) {
        const QPointF currentVector = event->scenePos() - rotationCenterScene_;
        const double currentAngle = std::atan2(currentVector.y(), currentVector.x());
        const double delta = currentAngle - startAngle_;
        setRotation(startRotation_ + qRadiansToDegrees(delta));
        const QPointF newPivotScene = mapToScene(rotationPivotLocal_);
        const QPointF shift = rotationPivotScene_ - newPivotScene;
        setPos(pos() + shift);
        startAngle_ = currentAngle;
        startRotation_ = rotation();
        event->accept();
        return;
    }

    QGraphicsObject::mouseMoveEvent(event);
}

void RulerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (resizing_ || rotating_) {
        resizing_ = false;
        rotating_ = false;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    QGraphicsObject::mouseReleaseEvent(event);
}

void RulerItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    const QRectF bounds = boundingRect();
    const qreal edgeThreshold = 14.0;
    const QPointF localPos = event->pos();
    if (localPos.y() < bounds.top() + 10.0 || localPos.y() > bounds.bottom() - 10.0) {
        setCursor(Qt::SizeAllCursor);
        return;
    }
    if (std::abs(localPos.x() - bounds.left()) < edgeThreshold ||
        std::abs(localPos.x() - bounds.right()) < edgeThreshold) {
        setCursor(Qt::SizeHorCursor);
        return;
    }
    setCursor(Qt::OpenHandCursor);
}

void RulerItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    setCursor(Qt::OpenHandCursor);
}
