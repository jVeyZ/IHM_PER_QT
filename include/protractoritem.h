#pragma once

#include <QGraphicsObject>
#include <QPainterPath>

class ProtractorItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit ProtractorItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setRadius(double radius);
    double radius() const;

private:
    double radius_ = 150.0;
    QPainterPath tickPath_;

    void rebuildTicks();
};
