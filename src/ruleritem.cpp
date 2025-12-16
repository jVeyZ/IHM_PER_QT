#include "ruleritem.h"

#include <QBrush>
#include <QCursor>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPainter>
#include <QPen>
#include <QPixmap>

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
    // Much wider and taller bounding rect to match the 3× scaled ruler
    return QRectF(-length_ / 2.0, -180.0, length_, 360.0);
}

void RulerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF rect = boundingRect();

    // Try to draw the new ruler SVG; fall back to old styled drawing if unavailable
    QPixmap pix(QStringLiteral(":/resources/images/ruler.svg"));
    if (!pix.isNull()) {
        // Draw the pixmap centered and scaled to fit the bounding rect while preserving aspect ratio
        const QSizeF targetSize = rect.size();
        const QPixmap scaled = pix.scaled(targetSize.toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPointF topLeft(rect.center().x() - scaled.width() / 2.0, rect.center().y() - scaled.height() / 2.0);
        const QRectF drawnRect(topLeft, QSizeF(scaled.width(), scaled.height()));
        // store drawn rect in local coordinates so mouse/hover checks use visible area (avoids invisible SVG margins)
        drawnRectLocal_ = drawnRect;
        painter->drawPixmap(drawnRect, scaled, QRectF(scaled.rect()));
        return;
    }
    // No pixmap: clear drawn rect so we use boundingRect for interactions
    drawnRectLocal_ = QRectF();

    // Fallback: draw a simple ruler look
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
    // Enforce a higher minimum so the ruler stays large (1.5× previous)
    length_ = std::max(length, 540.0);
}

double RulerItem::length() const {
    return length_;
}

void RulerItem::resetState() {
    // Start the ruler at the new default (1.5× the previous size)
    setLength(2340.0);
    setRotation(0.0);
    setPos(QPointF(0.0, 0.0));
}

void RulerItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsObject::mousePressEvent(event);
        return;
    }

    // Clear any stale rotating/resizing state in case a previous interaction
    // ended without delivering a proper release event.
    if (rotating_ || resizing_) {
        rotating_ = false;
        resizing_ = false;
        setCursor(Qt::OpenHandCursor);
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
    }

    const QRectF sourceRect = (!drawnRectLocal_.isNull() ? drawnRectLocal_ : boundingRect());
    const qreal edgeThreshold = 14.0;
    const QPointF localPos = event->pos();
    const bool nearTop = localPos.y() < sourceRect.top() + 10.0;
    const bool nearBottom = localPos.y() > sourceRect.bottom() - 10.0;
    const double edgeRegion = sourceRect.width() * 0.2;
    const bool nearLeftEdge = std::abs(localPos.x() - sourceRect.left()) < edgeRegion;
    const bool nearRightEdge = std::abs(localPos.x() - sourceRect.right()) < edgeRegion;

    startScenePos_ = event->scenePos();
    startRotation_ = rotation();
    rotationCenterScene_ = mapToScene(QPointF(0, 0));
    rotationStartVector_ = event->scenePos() - rotationCenterScene_;
    startAngle_ = std::atan2(rotationStartVector_.y(), rotationStartVector_.x());
    lastPointerScenePos_ = event->scenePos();

    // Only allow rotation (by dragging near top/bottom + near one edge) or move. Resizing is disabled.
    if ((nearTop || nearBottom) && (nearLeftEdge || nearRightEdge)) {
        rotating_ = true;
        resizeEdge_ = ResizeEdge::None;
        setCursor(Qt::SizeAllCursor);
        if (scene() && scene()->mouseGrabberItem() == nullptr) {
            grabMouse();
        }
        const double pivotX = nearLeftEdge ? sourceRect.right() : sourceRect.left();
        rotationPivotLocal_ = QPointF(pivotX, 0.0);
        rotationPivotScene_ = mapToScene(rotationPivotLocal_);
        event->accept();
        return;
    }

    // Start manual move handling (instead of relying on QGraphicsObject::mousePressEvent)
    resizeEdge_ = ResizeEdge::None;
    moving_ = true;
    lastPointerScenePos_ = event->scenePos();
    setCursor(Qt::ClosedHandCursor);
    event->accept();
}

void RulerItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    // Resizing disabled: skip resize handling and defer to default movement/rotation behavior
    if (resizing_) {
        resizing_ = false;
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

    if (moving_) {
        const QPointF delta = event->scenePos() - lastPointerScenePos_;
        setPos(pos() + delta);
        lastPointerScenePos_ = event->scenePos();
        event->accept();
        return;
    }

    // Fallback: default behavior
    QGraphicsObject::mouseMoveEvent(event);
}

void RulerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (resizing_ || rotating_) {
        resizing_ = false;
        rotating_ = false;
        setCursor(Qt::OpenHandCursor);
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
        event->accept();
        return;
    }

    if (moving_) {
        moving_ = false;
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
    // Do not show horizontal resize cursor; resizing is disabled
    // Use drawn rect (if available) for hover checks so cursor reflects visible area
    const QRectF sourceRect = (!drawnRectLocal_.isNull() ? drawnRectLocal_ : bounds);
    if (localPos.y() < sourceRect.top() + 10.0 || localPos.y() > sourceRect.bottom() - 10.0) {
        setCursor(Qt::SizeAllCursor);
        return;
    }
    setCursor(Qt::OpenHandCursor);
}

void RulerItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    setCursor(Qt::OpenHandCursor);
}

void RulerItem::cancelInteraction() {
    resizing_ = false;
    if (rotating_) {
        rotating_ = false;
        setCursor(Qt::OpenHandCursor);
        if (scene() && scene()->mouseGrabberItem() == this) {
            ungrabMouse();
        }
    }
}
