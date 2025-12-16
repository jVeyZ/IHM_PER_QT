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
#include "compassitem.h"

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

    void setProtractorVisible(bool visible, const QPointF &viewportCenter = QPointF());
    void setRulerVisible(bool visible, const QPointF &viewportCenter = QPointF());
    void setCompassVisible(bool visible, const QPointF &viewportCenter = QPointF());
    void cancelCompassDrag();
    void cancelRulerInteraction();

    // Try to start a pivot drag or handle drag on the compass if the scene
    // position targets it. Returns true if a drag was started.
    bool beginCompassPivotDragIfTarget(const QPointF &scenePos);
    bool beginCompassHandleDragIfTarget(const QPointF &scenePos);
    bool beginCompassRotationIfTarget(const QPointF &scenePos);

    void placeText(const QPointF &scenePos, const QString &text);
    void toggleExtremesForSelection();
    void clearMarks();
    bool isProtractorAt(const QPointF &scenePos) const;
    bool isRulerAt(const QPointF &scenePos) const;
    bool isCompassAt(const QPointF &scenePos) const;
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
    QPointer<CompassItem> compass_;
    bool compassEnabled_ = false;

    QPointF startPoint_;

    // Line drafting
    bool lineDrafting_ = false;
    QGraphicsPathItem *linePreviewPath_ = nullptr;
    QPainterPath currentLinePath_;

    // Arc drafting
    enum class ArcStage { None, CenterSet, StartSet };
    ArcStage arcStage_ = ArcStage::None;
    QPointF arcCenter_;
    double arcStartAngle_ = 0.0;
    double arcRadius_ = 0.0;
    QGraphicsEllipseItem *arcHelperCircle_ = nullptr;
    QGraphicsPathItem *arcPreview_ = nullptr;
    // For continuous arc drawing we accumulate signed rotation deltas so
    // the user can draw arcs larger than 180° (and full circles) by rotating.
    double arcAccumulatedSpan_ = 0.0;
    double arcLastAngle_ = 0.0;
    // (no longer storing/restoring compass rotation; the compass rotation persists)
    // (legacy circle drafting was removed; drawing now uses Arc stages when starting from compass handle)

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
