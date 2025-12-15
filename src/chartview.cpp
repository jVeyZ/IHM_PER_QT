#include "chartview.h"

#include <QWheelEvent>
#include <QCursor>
#include <QMouseEvent>
#include <QPoint>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QRegion>

#include <QtGlobal>

#include <algorithm>

ChartView::ChartView(QWidget *parent)
    : QGraphicsView(parent) {
    setRenderHint(QPainter::Antialiasing, true);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet(QStringLiteral("background-color: #f6f8fa;"));
    viewport()->setStyleSheet(QStringLiteral("background-color: #f6f8fa;"));
}

void ChartView::setZoomStep(double factor) {
    zoomStep_ = factor;
}

void ChartView::setZoomRange(double minFactor, double maxFactor) {
    minZoomFactor_ = minFactor;
    maxZoomFactor_ = maxFactor;
}

void ChartView::setHandNavigationEnabled(bool enabled) {
    if (handNavigationEnabled_ == enabled) {
        return;
    }
    handNavigationEnabled_ = enabled;
    setDragMode(QGraphicsView::NoDrag);
    panning_ = false;
    if (enabled) {
        viewport()->setCursor(Qt::OpenHandCursor);
    } else {
        viewport()->unsetCursor();
    }
}

void ChartView::zoomIn() {
    applyZoom(zoomStep_, mapToScene(rect().center()));
}

void ChartView::zoomOut() {
    applyZoom(1.0 / zoomStep_, mapToScene(rect().center()));
}

void ChartView::resetZoom() {
    const double factor = 1.0 / currentZoomFactor_;
    applyZoom(factor, mapToScene(rect().center()));
}

void ChartView::wheelEvent(QWheelEvent *event) {
    const bool modifierZoom = event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);
    const bool hasPixelDelta = !event->pixelDelta().isNull();
    const bool hasAngleDelta = !event->angleDelta().isNull();
    const bool trackpadZoom = !modifierZoom && hasPixelDelta;
    const bool mouseWheelZoom = !hasPixelDelta && hasAngleDelta;

    if (modifierZoom || trackpadZoom || mouseWheelZoom) {
        event->accept();
        const double direction = hasPixelDelta ? event->pixelDelta().y() : event->angleDelta().y();
        const double factor = direction > 0 ? zoomStep_ : 1.0 / zoomStep_;
        applyZoom(factor, mapToScene(event->position().toPoint()));
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void ChartView::mousePressEvent(QMouseEvent *event) {
    if (handNavigationEnabled_ && event->button() == Qt::LeftButton) {
        if (auto *cs = qobject_cast<ChartScene *>(scene())) {
            const QPointF scenePos = mapToScene(event->pos());
            const bool isRulerTarget = cs->isRulerAt(scenePos);
            const bool isProtractorTarget = cs->isProtractorAt(scenePos);
            if (isRulerTarget || isProtractorTarget) {
                const DragMode savedMode = dragMode();
                setDragMode(QGraphicsView::NoDrag);
                QGraphicsView::mousePressEvent(event);
                setDragMode(savedMode);
                return;
            }
        }
        lastPanPoint_ = event->pos();
        panning_ = true;
        viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void ChartView::mouseMoveEvent(QMouseEvent *event) {
    if (handNavigationEnabled_ && panning_ && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->pos() - lastPanPoint_;
        lastPanPoint_ = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event) {
    if (handNavigationEnabled_ && panning_ && event->button() == Qt::LeftButton) {
        panning_ = false;
        viewport()->setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void ChartView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    updateViewportMask();
}

void ChartView::paintEvent(QPaintEvent *event) {
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw rounded background
    QPainterPath path;
    path.addRoundedRect(viewport()->rect(), borderRadius_, borderRadius_);
    painter.setClipPath(path);
    painter.fillPath(path, QColor(0xf6, 0xf8, 0xfa));
    
    // Call base implementation for scene rendering
    QGraphicsView::paintEvent(event);
}

void ChartView::updateViewportMask() {
    QPainterPath path;
    path.addRoundedRect(viewport()->rect(), borderRadius_, borderRadius_);
    QRegion mask(path.toFillPolygon().toPolygon());
    viewport()->setMask(mask);
}

void ChartView::applyZoom(double factor, const QPointF &anchorScenePos) {
    const double newFactor = std::clamp(currentZoomFactor_ * factor, minZoomFactor_, maxZoomFactor_);
    const double scaleFactor = newFactor / currentZoomFactor_;
    if (qFuzzyCompare(scaleFactor, 1.0)) {
        return;
    }
    const QPoint viewAnchor = mapFromScene(anchorScenePos);
    currentZoomFactor_ = newFactor;
    scale(scaleFactor, scaleFactor);
    const QPointF newScenePos = mapToScene(viewAnchor);
    const QPointF delta = newScenePos - anchorScenePos;
    translate(delta.x(), delta.y());
}
