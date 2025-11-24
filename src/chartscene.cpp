#include "chartscene.h"

#include <QAbstractGraphicsShapeItem>
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QKeyEvent>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QPointer>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {
double toSceneAngle(const QPointF &center, const QPointF &point) {
    const double dx = point.x() - center.x();
    const double dy = center.y() - point.y();
    double angle = qRadiansToDegrees(std::atan2(dy, dx));
    if (angle < 0.0) {
        angle += 360.0;
    }
    return angle;
}

double distance(const QPointF &a, const QPointF &b) {
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}
}

ChartScene::ChartScene(QObject *parent)
    : QGraphicsScene(parent) {
    setItemIndexMethod(QGraphicsScene::NoIndex);

    protractor_ = new ProtractorItem();
    protractor_->setVisible(false);
    protractor_->setZValue(50);
    addItem(protractor_);

    ruler_ = new RulerItem();
    ruler_->setVisible(false);
    ruler_->setZValue(40);
    addItem(ruler_);

    connect(this, &QGraphicsScene::selectionChanged, this, [this]() {
        if (currentTool_ == Tool::Select) {
            for (auto *item : selectedItems()) {
                item->setZValue(item->zValue() + 0.1);
            }
        }
    });
}

void ChartScene::setTool(Tool tool) {
    currentTool_ = tool;
    awaitingText_ = false;
}

ChartScene::Tool ChartScene::tool() const {
    return currentTool_;
}

void ChartScene::setCurrentColor(const QColor &color) {
    currentColor_ = color;
    const auto itemsSelected = selectedItems();
    for (auto *item : itemsSelected) {
        applyColorToItem(item, color);
    }
    if (crosshairHorizontal_) {
        applyColorToItem(crosshairHorizontal_, color);
    }
    if (crosshairVertical_) {
        applyColorToItem(crosshairVertical_, color);
    }
}

QColor ChartScene::currentColor() const {
    return currentColor_;
}

void ChartScene::setLineWidth(int width) {
    lineWidth_ = std::clamp(width, 1, 12);
}

int ChartScene::lineWidth() const {
    return lineWidth_;
}

void ChartScene::setPixelsPerNauticalMile(double value) {
    pixelsPerNauticalMile_ = std::clamp(value, 10.0, 1000.0);
}

double ChartScene::pixelsPerNauticalMile() const {
    return pixelsPerNauticalMile_;
}

void ChartScene::setBackgroundPixmap(const QPixmap &pixmap) {
    if (background_) {
        removeItem(background_);
        delete background_;
        background_ = nullptr;
    }
    background_ = addPixmap(pixmap);
    background_->setZValue(-100.0);
    background_->setTransformationMode(Qt::SmoothTransformation);
    background_->setEnabled(false);

    setSceneRect(background_->boundingRect());

    if (protractor_) {
        protractor_->setPos(sceneRect().center());
    }
    if (ruler_) {
        ruler_->setPos(sceneRect().center() + QPointF(0, 120));
    }
}

void ChartScene::setProtractorVisible(bool visible) {
    if (protractor_) {
        protractor_->setVisible(visible);
        if (visible) {
            protractor_->setPos(sceneRect().center());
        }
    }
}

void ChartScene::setRulerVisible(bool visible) {
    if (ruler_) {
        ruler_->setVisible(visible);
        if (visible) {
            ruler_->resetState();
            ruler_->setPos(sceneRect().center() + QPointF(0.0, 180.0));
        }
    }
}

bool ChartScene::isRulerAt(const QPointF &scenePos) const {
    if (!ruler_ || !ruler_->isVisible()) {
        return false;
    }

    const QPointF localPos = ruler_->mapFromScene(scenePos);
    return ruler_->boundingRect().contains(localPos);
}

void ChartScene::placeText(const QPointF &scenePos, const QString &text) {
    if (text.trimmed().isEmpty()) {
        awaitingText_ = false;
        return;
    }

    auto *item = addText(text, QFont("Gill Sans", 12, QFont::Bold));
    item->setDefaultTextColor(currentColor_);
    item->setPos(scenePos);
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
    item->setFlag(QGraphicsItem::ItemIsMovable, true);
    item->setData(0, TextItem);
    item->setZValue(30.0);

    awaitingText_ = false;
    updateMarkCount();
}

void ChartScene::toggleExtremesForSelection() {
    const QRectF rect = sceneRect();
    for (auto *item : selectedItems()) {
        if (item->data(0).toInt() != PointItem) {
            continue;
        }

        if (extremesByPoint_.contains(item)) {
            auto lines = extremesByPoint_.take(item);
            if (lines.horizontal) {
                removeItem(lines.horizontal);
                delete lines.horizontal;
            }
            if (lines.vertical) {
                removeItem(lines.vertical);
                delete lines.vertical;
            }
            continue;
        }

        const QPointF pos = item->scenePos();
        auto *hLine = addLine(rect.left(), pos.y(), rect.right(), pos.y(), QPen(QColor(11, 61, 112), 1, Qt::DashLine));
        auto *vLine = addLine(pos.x(), rect.top(), pos.x(), rect.bottom(), QPen(QColor(11, 61, 112), 1, Qt::DashLine));
        hLine->setZValue(10.0);
        vLine->setZValue(10.0);

        extremesByPoint_.insert(item, {hLine, vLine});
    }
}

void ChartScene::clearMarks() {
    clearCrosshair();
    const auto allItems = items();
    for (auto *item : allItems) {
        if (isProtectedItem(item)) {
            continue;
        }
        removeItemAndChildren(item);
        delete item;
    }
    extremesByPoint_.clear();
    updateMarkCount();
    emit statusMessage(tr("Carta limpia"));
}

void ChartScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    const QPointF pos = event->scenePos();

    const auto targetedItems = items(pos);
    for (auto *item : targetedItems) {
        if (item == ruler_ || item->parentItem() == ruler_) {
            QGraphicsScene::mousePressEvent(event);
            return;
        }
    }

    switch (currentTool_) {
    case Tool::Select:
        QGraphicsScene::mousePressEvent(event);
        break;
    case Tool::Point: {
        const double radius = 6.0;
        auto *item = addEllipse(QRectF(pos.x() - radius, pos.y() - radius, radius * 2.0, radius * 2.0),
                                QPen(currentColor_, lineWidth_), QBrush(currentColor_));
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setData(0, PointItem);
        item->setZValue(20.0);
        updateMarkCount();
        break;
    }
    case Tool::Line: {
        lineDrafting_ = true;
        startPoint_ = pos;
        if (linePreview_) {
            removeItem(linePreview_);
            delete linePreview_;
        }
        linePreview_ = addLine(QLineF(pos, pos), QPen(currentColor_, lineWidth_, Qt::SolidLine, Qt::RoundCap));
        linePreview_->setZValue(25.0);
        break;
    }
    case Tool::Arc: {
        if (arcStage_ == ArcStage::None) {
            arcCenter_ = pos;
            arcStage_ = ArcStage::CenterSet;
            emit statusMessage(tr("Seleccione el inicio del arco."));
        } else if (arcStage_ == ArcStage::CenterSet) {
            const double radius = distance(arcCenter_, pos);
            if (radius < 4.0) {
                emit statusMessage(tr("Radio demasiado pequeño, seleccione un punto más alejado."));
                return;
            }
            arcRadius_ = radius;
            arcStartAngle_ = toSceneAngle(arcCenter_, pos);

            if (arcHelperCircle_) {
                removeItem(arcHelperCircle_);
                delete arcHelperCircle_;
            }
            arcHelperCircle_ = addEllipse(QRectF(arcCenter_.x() - arcRadius_, arcCenter_.y() - arcRadius_,
                                                 arcRadius_ * 2.0, arcRadius_ * 2.0),
                                          QPen(QColor(31, 119, 180, 120), 1, Qt::DashLine), Qt::NoBrush);
            arcHelperCircle_->setZValue(18.0);

            if (arcPreview_) {
                removeItem(arcPreview_);
                delete arcPreview_;
            }
            arcPreview_ = addPath(QPainterPath(), QPen(currentColor_, lineWidth_ + 1, Qt::SolidLine, Qt::RoundCap));
            arcPreview_->setZValue(26.0);
            arcStage_ = ArcStage::StartSet;
            emit statusMessage(tr("Arrastre para definir el arco y suelte para finalizar."));
        }
        break;
    }
    case Tool::Text: {
        if (!awaitingText_) {
            awaitingText_ = true;
            pendingTextPos_ = pos;
            emit textRequested(pos);
        }
        break;
    }
    case Tool::Distance: {
        distanceDrafting_ = true;
        startPoint_ = pos;
        if (distancePreview_) {
            removeItem(distancePreview_);
            delete distancePreview_;
        }
        distancePreview_ = new DistanceItem();
        distancePreview_->setData(0, DistanceItemType);
        addItem(distancePreview_);
        break;
    }
    case Tool::Eraser: {
        const auto itemsAt = items(pos);
        for (auto *item : itemsAt) {
            if (item == crosshairHorizontal_ || item == crosshairVertical_) {
                clearCrosshair();
                updateMarkCount();
                emit statusMessage(tr("Mira eliminada"));
                break;
            }
            if (isProtectedItem(item)) {
                continue;
            }
            removeItemAndChildren(item);
            delete item;
            extremesByPoint_.remove(item);
            updateMarkCount();
            emit statusMessage(tr("Marca eliminada"));
            break;
        }
        break;
    }
    case Tool::Crosshair: {
        placeCrosshair(pos);
        break;
    }
    }
}

void ChartScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    const QPointF pos = event->scenePos();

    if (lineDrafting_ && linePreview_) {
        linePreview_->setLine(QLineF(startPoint_, pos));
        event->accept();
        return;
    }

    if (arcStage_ == ArcStage::StartSet && arcPreview_) {
        const double angle = toSceneAngle(arcCenter_, pos);
        double span = angle - arcStartAngle_;
        if (span <= 0.0) {
            span += 360.0;
        }
        QPainterPath path;
        const QRectF rect(arcCenter_.x() - arcRadius_, arcCenter_.y() - arcRadius_, arcRadius_ * 2.0, arcRadius_ * 2.0);
        path.arcMoveTo(rect, arcStartAngle_);
        path.arcTo(rect, arcStartAngle_, span);
        arcPreview_->setPath(path);
        event->accept();
        return;
    }

    if (distanceDrafting_ && distancePreview_) {
        distancePreview_->updateGeometry(startPoint_, pos, currentColor_, lineWidth_ + 1, pixelsPerNauticalMile_);
        event->accept();
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void ChartScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    const QPointF pos = event->scenePos();

    if (lineDrafting_ && linePreview_) {
        lineDrafting_ = false;
        const QLineF line(startPoint_, pos);
        if (line.length() > 3.0) {
            auto *item = addLine(line, QPen(currentColor_, lineWidth_, Qt::SolidLine, Qt::RoundCap));
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
            item->setFlag(QGraphicsItem::ItemIsMovable, true);
            item->setData(0, LineItem);
            item->setZValue(22.0);
            updateMarkCount();
        }
        removeItem(linePreview_);
        delete linePreview_;
        linePreview_ = nullptr;
        event->accept();
        return;
    }

    if (arcStage_ == ArcStage::StartSet && arcPreview_) {
        const double angle = toSceneAngle(arcCenter_, pos);
        double span = angle - arcStartAngle_;
        if (span <= 0.0) {
            span += 360.0;
        }
        if (span > 0.5) {
            auto *item = addPath(arcPreview_->path(), QPen(currentColor_, lineWidth_, Qt::SolidLine, Qt::RoundCap));
            item->setFlag(QGraphicsItem::ItemIsSelectable, true);
            item->setFlag(QGraphicsItem::ItemIsMovable, true);
            item->setData(0, ArcItem);
            item->setZValue(24.0);
            updateMarkCount();
        }
        removeItem(arcPreview_);
        delete arcPreview_;
        arcPreview_ = nullptr;
        if (arcHelperCircle_) {
            removeItem(arcHelperCircle_);
            delete arcHelperCircle_;
            arcHelperCircle_ = nullptr;
        }
        arcStage_ = ArcStage::None;
        event->accept();
        return;
    }

    if (distanceDrafting_ && distancePreview_) {
        distanceDrafting_ = false;
        distancePreview_->updateGeometry(startPoint_, pos, currentColor_, lineWidth_ + 1, pixelsPerNauticalMile_);
        emit distanceMeasured(distancePreview_->pixels(), distancePreview_->nauticalMiles());
        updateMarkCount();
        distancePreview_ = nullptr;
        event->accept();
        return;
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void ChartScene::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        cancelLineDraft();
        cancelArcDraft();
        cancelDistanceDraft();
        awaitingText_ = false;
        event->accept();
        return;
    }
    QGraphicsScene::keyPressEvent(event);
}

void ChartScene::applyColorToItem(QGraphicsItem *item, const QColor &color) {
    switch (item->data(0).toInt()) {
    case PointItem: {
        if (auto *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem *>(item)) {
            ellipse->setBrush(color);
            ellipse->setPen(QPen(color.darker(120), lineWidth_));
        }
        break;
    }
    case LineItem: {
        if (auto *line = qgraphicsitem_cast<QGraphicsLineItem *>(item)) {
            auto pen = line->pen();
            pen.setColor(color);
            pen.setWidth(lineWidth_);
            pen.setCapStyle(Qt::RoundCap);
            line->setPen(pen);
        }
        break;
    }
    case ArcItem: {
        if (auto *path = qgraphicsitem_cast<QGraphicsPathItem *>(item)) {
            auto pen = path->pen();
            pen.setColor(color);
            pen.setWidth(lineWidth_);
            pen.setCapStyle(Qt::RoundCap);
            path->setPen(pen);
        }
        break;
    }
    case TextItem: {
        if (auto *text = qgraphicsitem_cast<QGraphicsTextItem *>(item)) {
            text->setDefaultTextColor(color);
        }
        break;
    }
    case DistanceItemType: {
        if (auto *distanceItem = dynamic_cast<DistanceItem *>(item)) {
            distanceItem->updateGeometry(distanceItem->startPoint(),
                                         distanceItem->endPoint(),
                                         color, lineWidth_, pixelsPerNauticalMile_);
        }
        break;
    }
    case CrosshairItem: {
        if (auto *line = qgraphicsitem_cast<QGraphicsLineItem *>(item)) {
            auto pen = line->pen();
            pen.setColor(color);
            pen.setWidth(lineWidth_);
            pen.setStyle(Qt::DashLine);
            line->setPen(pen);
        }
        break;
    }
    default:
        break;
    }
}

void ChartScene::removeItemAndChildren(QGraphicsItem *item) {
    if (!item) {
        return;
    }

    if (item == crosshairHorizontal_) {
        crosshairHorizontal_ = nullptr;
    } else if (item == crosshairVertical_) {
        crosshairVertical_ = nullptr;
    }

    if (extremesByPoint_.contains(item)) {
        auto lines = extremesByPoint_.take(item);
        if (lines.horizontal) {
            removeItem(lines.horizontal);
            delete lines.horizontal;
        }
        if (lines.vertical) {
            removeItem(lines.vertical);
            delete lines.vertical;
        }
    }

    const auto children = item->childItems();
    for (auto *child : children) {
        removeItemAndChildren(child);
    }
    removeItem(item);
}

bool ChartScene::isProtectedItem(QGraphicsItem *item) const {
    if (!item) {
        return true;
    }
    if (item == background_ || item == protractor_ || item == ruler_) {
        return true;
    }
    if (item->parentItem() == protractor_ || item->parentItem() == ruler_) {
        return true;
    }
    return false;
}

void ChartScene::updateMarkCount() {
    int count = 0;
    for (auto *item : items()) {
        const int type = item->data(0).toInt();
        if (type >= PointItem && type <= DistanceItemType) {
            ++count;
        }
    }
    emit markCountChanged(count);
}

void ChartScene::cancelArcDraft() {
    arcStage_ = ArcStage::None;
    if (arcPreview_) {
        removeItem(arcPreview_);
        delete arcPreview_;
        arcPreview_ = nullptr;
    }
    if (arcHelperCircle_) {
        removeItem(arcHelperCircle_);
        delete arcHelperCircle_;
        arcHelperCircle_ = nullptr;
    }
}

void ChartScene::cancelLineDraft() {
    lineDrafting_ = false;
    if (linePreview_) {
        removeItem(linePreview_);
        delete linePreview_;
        linePreview_ = nullptr;
    }
}

void ChartScene::cancelDistanceDraft() {
    distanceDrafting_ = false;
    if (distancePreview_) {
        removeItem(distancePreview_);
        delete distancePreview_;
        distancePreview_ = nullptr;
    }
}

void ChartScene::clearCrosshair() {
    if (crosshairHorizontal_) {
        removeItem(crosshairHorizontal_);
        delete crosshairHorizontal_;
        crosshairHorizontal_ = nullptr;
    }
    if (crosshairVertical_) {
        removeItem(crosshairVertical_);
        delete crosshairVertical_;
        crosshairVertical_ = nullptr;
    }
}

void ChartScene::placeCrosshair(const QPointF &pos) {
    clearCrosshair();
    const QRectF rect = sceneRect();
    const QPen pen(currentColor_, lineWidth_, Qt::DashLine);
    crosshairHorizontal_ = addLine(rect.left(), pos.y(), rect.right(), pos.y(), pen);
    crosshairVertical_ = addLine(pos.x(), rect.top(), pos.x(), rect.bottom(), pen);
    if (crosshairHorizontal_) {
        crosshairHorizontal_->setZValue(32.0);
        crosshairHorizontal_->setData(0, CrosshairItem);
        crosshairHorizontal_->setFlag(QGraphicsItem::ItemIsSelectable, false);
        crosshairHorizontal_->setAcceptedMouseButtons(Qt::NoButton);
    }
    if (crosshairVertical_) {
        crosshairVertical_->setZValue(32.0);
        crosshairVertical_->setData(0, CrosshairItem);
        crosshairVertical_->setFlag(QGraphicsItem::ItemIsSelectable, false);
        crosshairVertical_->setAcceptedMouseButtons(Qt::NoButton);
    }
    updateMarkCount();
}
