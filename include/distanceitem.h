#pragma once

#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>

class DistanceItem : public QGraphicsItemGroup {
public:
    explicit DistanceItem(QGraphicsItem *parent = nullptr);

    void updateGeometry(const QPointF &start,
                        const QPointF &end,
                        const QColor &color,
                        int lineWidth,
                        double pixelsPerNauticalMile);

    double pixels() const;
    double nauticalMiles() const;
    QPointF startPoint() const;
    QPointF endPoint() const;

private:
    QGraphicsLineItem *line_ = nullptr;
    QGraphicsSimpleTextItem *label_ = nullptr;
    double pixels_ = 0.0;
    double nauticalMiles_ = 0.0;
    QPointF start_;
    QPointF end_;
};
