#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include "chartscene.h"

#include <QPoint>

class ChartView : public QGraphicsView {
    Q_OBJECT
public:
    explicit ChartView(QWidget *parent = nullptr);

    void setZoomStep(double factor);
    void setZoomRange(double minFactor, double maxFactor);
    void setHandNavigationEnabled(bool enabled);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void applyZoom(double factor, const QPointF &anchorScenePos);

    double zoomStep_ = 1.15;
    double minZoomFactor_ = 0.2;
    double maxZoomFactor_ = 5.0;
    double currentZoomFactor_ = 1.0;
    bool handNavigationEnabled_ = false;
    bool panning_ = false;
    QPoint lastPanPoint_;
};
