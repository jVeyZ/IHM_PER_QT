#include "protractoritem.h"

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QtMath>

#include <algorithm>
#include <cmath>

ProtractorItem::ProtractorItem(QGraphicsItem *parent)
    : QGraphicsObject(parent) {
    setFlag(ItemIsMovable, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    rebuildTicks();
}

QRectF ProtractorItem::boundingRect() const {
    const double height = radius_ + 24.0;
    return QRectF(-radius_, -height, radius_ * 2.0, height + 8.0);
}

void ProtractorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF outerRect(-radius_, -radius_, radius_ * 2.0, radius_ * 2.0);
    QPainterPath semiCircle;
    semiCircle.moveTo(-radius_, 0.0);
    semiCircle.arcTo(outerRect, 180.0, -180.0);
    semiCircle.lineTo(radius_, 0.0);
    semiCircle.closeSubpath();

    painter->setBrush(QColor(215, 236, 255, 180));
    painter->setPen(QPen(QColor(45, 109, 163), 2.0));
    painter->drawPath(semiCircle);

    painter->setPen(QPen(QColor(11, 61, 112), 1.5));
    painter->drawLine(QPointF(-radius_, 0.0), QPointF(radius_, 0.0));

    painter->setPen(QPen(QColor(11, 61, 112), 1.0));
    painter->drawPath(tickPath_);

    painter->setPen(QPen(QColor(31, 119, 180), 2.0));
    painter->drawLine(QPointF(0.0, 0.0), QPointF(0.0, -radius_));
}

void ProtractorItem::setRadius(double radius) {
    radius_ = std::clamp(radius, 60.0, 400.0);
    prepareGeometryChange();
    rebuildTicks();
}

double ProtractorItem::radius() const {
    return radius_;
}

void ProtractorItem::rebuildTicks() {
    tickPath_ = QPainterPath();
    for (int degree = 0; degree <= 180; degree += 2) {
        const double rad = qDegreesToRadians(static_cast<double>(degree));
        const double cosVal = std::cos(rad);
        const double sinVal = std::sin(rad);
        const double outer = radius_;
        const double inner = (degree % 10 == 0) ? radius_ - 18.0 : radius_ - 10.0;
        const QPointF start(outer * cosVal, -outer * sinVal);
        const QPointF end(inner * cosVal, -inner * sinVal);
        tickPath_.moveTo(start);
        tickPath_.lineTo(end);
    }
}
