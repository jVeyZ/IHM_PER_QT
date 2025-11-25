#include "protractoritem.h"

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
    svgRenderer_ = std::make_unique<QSvgRenderer>(QStringLiteral(":/resources/images/icon_protractor.svg"));
    if (svgRenderer_ && svgRenderer_->isValid()) {
        const QSizeF defaultSize = svgRenderer_->defaultSize();
        if (!defaultSize.isEmpty() && defaultSize.width() > 0.0) {
            svgAspectRatio_ = defaultSize.height() / defaultSize.width();
        }
    }
}

QRectF ProtractorItem::boundingRect() const {
    const double width = radius_ * 2.0;
    const double aspect = svgAspectRatio_ > 0.0 ? svgAspectRatio_ : 0.5;
    const double height = width * aspect;
    return QRectF(-radius_, -height, width, height);
}

void ProtractorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    if (!svgRenderer_ || !svgRenderer_->isValid()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, true);
    const double width = radius_ * 2.0;
    const double aspect = svgAspectRatio_ > 0.0 ? svgAspectRatio_ : 0.5;
    const double height = width * aspect;
    const QRectF targetRect(-radius_, -height, width, height);
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
