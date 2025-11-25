#pragma once

#include <QColor>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QHash>
#include <QPointer>

#include "distanceitem.h"
#include "protractoritem.h"
#include "ruleritem.h"

class ChartScene : public QGraphicsScene {
    Q_OBJECT
public:
    enum class Tool {
        Select,
        Point,
        Line,
        Arc,
        Text,
        Distance,
        Eraser,
        Crosshair
    };

    explicit ChartScene(QObject *parent = nullptr);

    void setTool(Tool tool);
    Tool tool() const;

    void setCurrentColor(const QColor &color);
    QColor currentColor() const;

    void setLineWidth(int width);
    int lineWidth() const;

    void setPixelsPerNauticalMile(double value);
    double pixelsPerNauticalMile() const;

    void setBackgroundPixmap(const QPixmap &pixmap);

    void setProtractorVisible(bool visible);
    void setRulerVisible(bool visible);

    void placeText(const QPointF &scenePos, const QString &text);
    void toggleExtremesForSelection();
    void clearMarks();
    bool isProtractorAt(const QPointF &scenePos) const;
    bool isRulerAt(const QPointF &scenePos) const;
    void clearCrosshair();

signals:
    void textRequested(const QPointF &scenePos);
    void distanceMeasured(double pixels, double nauticalMiles);
    void statusMessage(const QString &message);
    void markCountChanged(int count);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum ItemType {
        PointItem = 1,
        LineItem,
        ArcItem,
        TextItem,
        DistanceItemType,
        CrosshairItem
    };

    struct ExtremesLines {
        QGraphicsLineItem *horizontal = nullptr;
        QGraphicsLineItem *vertical = nullptr;
    };

    void applyColorToItem(QGraphicsItem *item, const QColor &color);
    void removeItemAndChildren(QGraphicsItem *item);
    bool isProtectedItem(QGraphicsItem *item) const;
    void updateMarkCount();
    void cancelArcDraft();
    void cancelLineDraft();
    void cancelDistanceDraft();
    void placeCrosshair(const QPointF &pos);

    Tool currentTool_ = Tool::Select;
    QColor currentColor_ = QColor("#000000");
    int lineWidth_ = 2;
    double pixelsPerNauticalMile_ = 120.0;

    QGraphicsPixmapItem *background_ = nullptr;
    QPointer<ProtractorItem> protractor_;
    QPointer<RulerItem> ruler_;

    QPointF startPoint_;

    // Line drafting
    bool lineDrafting_ = false;
    QGraphicsLineItem *linePreview_ = nullptr;

    // Arc drafting
    enum class ArcStage { None, CenterSet, StartSet };
    ArcStage arcStage_ = ArcStage::None;
    QPointF arcCenter_;
    double arcStartAngle_ = 0.0;
    double arcRadius_ = 0.0;
    QGraphicsEllipseItem *arcHelperCircle_ = nullptr;
    QGraphicsPathItem *arcPreview_ = nullptr;

    // Distance drafting
    bool distanceDrafting_ = false;
    DistanceItem *distancePreview_ = nullptr;

    // Text placement
    bool awaitingText_ = false;
    QPointF pendingTextPos_;

    QHash<QGraphicsItem *, ExtremesLines> extremesByPoint_;

    QGraphicsLineItem *crosshairHorizontal_ = nullptr;
    QGraphicsLineItem *crosshairVertical_ = nullptr;
};
