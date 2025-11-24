#include "distanceitem.h"

#include <QBrush>
#include <QPen>

DistanceItem::DistanceItem(QGraphicsItem *parent)
    : QGraphicsItemGroup(parent) {
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setHandlesChildEvents(false);

    line_ = new QGraphicsLineItem();
    label_ = new QGraphicsSimpleTextItem();

    addToGroup(line_);
    addToGroup(label_);
}

void DistanceItem::updateGeometry(const QPointF &start,
                                  const QPointF &end,
                                  const QColor &color,
                                  int lineWidth,
                                  double pixelsPerNauticalMile) {
    start_ = start;
    end_ = end;
    line_->setLine(QLineF(start, end));
    QPen pen(color, lineWidth);
    pen.setCapStyle(Qt::RoundCap);
    line_->setPen(pen);

    pixels_ = QLineF(start, end).length();
    nauticalMiles_ = pixelsPerNauticalMile <= 0.0 ? 0.0 : pixels_ / pixelsPerNauticalMile;

    label_->setText(QStringLiteral("%1 px | %2 NM")
                        .arg(pixels_, 0, 'f', 1)
                        .arg(nauticalMiles_, 0, 'f', 2));
    label_->setBrush(QBrush(color.darker(120)));

    const QPointF mid = (start + end) / 2.0;
    label_->setPos(mid + QPointF(6.0, -18.0));
}

double DistanceItem::pixels() const {
    return pixels_;
}

double DistanceItem::nauticalMiles() const {
    return nauticalMiles_;
}

QPointF DistanceItem::startPoint() const {
    return start_;
}

QPointF DistanceItem::endPoint() const {
    return end_;
}
