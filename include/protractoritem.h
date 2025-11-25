#pragma once

#include <QGraphicsObject>
#include <QtSvg/QSvgRenderer>
#include <memory>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
#include <memory>

class QSvgRenderer;

class ProtractorItem : public QGraphicsObject {
    Q_OBJECT
public:
    explicit ProtractorItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setRadius(double radius);
    double radius() const;

private:
    double radius_ = 200.0;
    double minRadius_ = 120.0;
    double maxRadius_ = 400.0;
    double svgAspectRatio_ = 0.5;
    std::unique_ptr<QSvgRenderer> svgRenderer_;
};
