#include "mainwindow.h"

#include "profiledialog.h"
#include "resultsdialog.h"

#include <QActionGroup>
#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QAbstractItemView>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QFont>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QCursor>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QPen>
#include <QRegularExpression>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTimer>
#include <QTextOption>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QMargins>
#include <QLocale>
#include <QMap>

#include <algorithm>
#include <cmath>
#include <QtGlobal>
#include <random>

namespace {
constexpr int kAvatarIconSize = 40;
constexpr int kAvatarPreviewSize = 96;
constexpr auto kLightThemePath = ":/styles/modern_light.qss";
constexpr auto kFallbackThemePath = ":/styles/lightblue.qss";
constexpr auto kDefaultAvatarPath = ":/resources/images/default_avatar.svg";
constexpr int kProblemPaneDefaultMinWidth = 360;
constexpr auto kCorrectAnswerStyle =
    "color: #1f7a4d; font-weight: 600; background-color: rgba(63,185,80,0.18); border-radius: 10px; padding: 6px 10px;";
constexpr auto kIncorrectAnswerStyle =
    "color: #b00020; font-weight: 600; background-color: rgba(248,113,113,0.18); border-radius: 10px; padding: 6px 10px;";
constexpr int kMaxStatsChartPoints = 12;
constexpr int kMaxStatsTableRows = 8;
}

class StatsTrendWidget : public QWidget {
public:
    struct BarData {
        QString label;
        double value = 0.0;
    };

    explicit StatsTrendWidget(QWidget *parent = nullptr)
        : QWidget(parent) {
        setMinimumHeight(220);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_StyledBackground, true);
        setObjectName("StatsChartWidget");
    }

    void setBars(const QVector<BarData> &bars) {
        bars_ = bars;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRectF canvas = rect().adjusted(32, 16, -48, -48);

        QPen axisPen(palette().mid().color(), 1.0);
        axisPen.setCapStyle(Qt::SquareCap);
        painter.setPen(axisPen);
        painter.drawLine(canvas.bottomLeft(), canvas.bottomRight());
        painter.drawLine(canvas.bottomLeft(), canvas.topLeft());

        const QVector<int> ticks{0, 25, 50, 75, 100};
        QPen gridPen(axisPen);
        gridPen.setStyle(Qt::DotLine);
        gridPen.setColor(axisPen.color().lighter(135));
        QFont labelFont = painter.font();
        const qreal currentSize = labelFont.pointSizeF();
        if (currentSize > 0.0) {
            labelFont.setPointSizeF(qMax<qreal>(currentSize - 1.0, 1.0));
        }
        painter.setFont(labelFont);
        for (int tick : ticks) {
            const double y = canvas.bottom() - (tick / 100.0) * canvas.height();
            painter.setPen(gridPen);
            painter.drawLine(QPointF(canvas.left(), y), QPointF(canvas.right(), y));
            painter.setPen(palette().mid().color());
            painter.drawText(QRectF(canvas.left() - 32, y - 8, 28, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(tick));
        }

        if (bars_.isEmpty()) {
            painter.setPen(palette().mid().color());
            painter.drawText(canvas, Qt::AlignCenter, QObject::tr("Sin datos todavía"));
            return;
        }

        const int count = bars_.size();
        const double slotWidth = canvas.width() / qMax(1, count);
        const double barWidth = qMin(40.0, slotWidth * 0.6);
        const QColor barColor("#1f6feb");
        const QColor negativeColor("#b00020");

        painter.setPen(Qt::NoPen);

        for (int index = 0; index < count; ++index) {
            const auto &bar = bars_.at(index);
            const double value = std::clamp(bar.value, 0.0, 100.0);
            const double height = (value / 100.0) * canvas.height();
            const double xCenter = canvas.left() + slotWidth * index + slotWidth / 2.0;
            const QRectF barRect(xCenter - barWidth / 2.0, canvas.bottom() - height, barWidth, height);

            painter.setBrush(barColor);
            painter.drawRoundedRect(barRect, 6, 6);

            painter.setPen(palette().mid().color());
            painter.drawText(QRectF(barRect.left(), canvas.bottom() + 4, barRect.width(), 18), Qt::AlignCenter, bar.label);

            painter.setPen(value >= 50.0 ? barColor : negativeColor);
            painter.drawText(QRectF(barRect.left(), barRect.top() - 20, barRect.width(), 18), Qt::AlignCenter, QString::number(value, 'f', 0) + QLatin1String("%"));

            painter.setPen(Qt::NoPen);
        }
    }

private:
    QVector<BarData> bars_;
};

class StatsPieWidget : public QWidget {
public:
    explicit StatsPieWidget(QWidget *parent = nullptr)
        : QWidget(parent) {
        setMinimumHeight(220);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        setAttribute(Qt::WA_StyledBackground, true);
        setObjectName("StatsChartWidget");
    }

    void setValues(int correct, int incorrect) {
        if (correct_ == correct && incorrect_ == incorrect) {
            return;
        }
        correct_ = qMax(0, correct);
        incorrect_ = qMax(0, incorrect);
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QWidget::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRectF paddedRect = rect().adjusted(32, 24, -32, -24);
        if (paddedRect.width() <= 0 || paddedRect.height() <= 0) {
            return;
        }

        constexpr double legendWidth = 150.0;
        constexpr double legendGap = 24.0;
        QRectF legendRect = paddedRect;
        legendRect.setRight(legendRect.left() + legendWidth);

        QRectF chartRect = paddedRect.adjusted(legendWidth + legendGap, 0, 0, 0);
        if (chartRect.width() <= 0 || chartRect.height() <= 0) {
            return;
        }

        const double diameter = qMin(chartRect.width(), chartRect.height());
        const QPointF center = chartRect.center();
        const QRectF pieRect(center.x() - diameter / 2.0,
                             center.y() - diameter / 2.0,
                             diameter,
                             diameter);

        const QColor correctColor("#3fb950");
        const QColor incorrectColor("#f85149");

        const int total = correct_ + incorrect_;
        if (total <= 0) {
            painter.setPen(palette().mid().color());
            painter.drawText(chartRect, Qt::AlignCenter, QObject::tr("Sin datos"));
            return;
        }

        painter.setPen(Qt::NoPen);
        int startAngle = 90 * 16;
        const double correctRatio = static_cast<double>(correct_) / total;
        const int totalSpan = 360 * 16;
        const int correctSpan = static_cast<int>(qRound(correctRatio * totalSpan));
        const int incorrectSpan = totalSpan - correctSpan;

        painter.setBrush(correctColor);
        painter.drawPie(pieRect, startAngle, -correctSpan);
        painter.setBrush(incorrectColor);
        painter.drawPie(pieRect, startAngle - correctSpan, -incorrectSpan);

        const auto drawLegend = [&](const QPointF &origin, const QColor &color, const QString &text) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            QRectF swatch(origin, QSizeF(14, 14));
            painter.drawRoundedRect(swatch, 4, 4);
            painter.setPen(palette().text().color());
            painter.drawText(QRectF(swatch.right() + 8, swatch.top() - 4, legendWidth - 40, 24),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             text);
        };

        const QLocale locale;
        constexpr double legendRowHeight = 28.0;
        constexpr double legendRowSpacing = 12.0;
        const double legendBlockHeight = legendRowHeight * 2 + legendRowSpacing;
        const double legendStartY = legendRect.top() + (legendRect.height() - legendBlockHeight) / 2.0;
        drawLegend(QPointF(legendRect.left(), legendStartY),
                   correctColor,
                   QObject::tr("Correctas (%1)").arg(locale.toString(correct_)));
        drawLegend(QPointF(legendRect.left(), legendStartY + legendRowHeight + legendRowSpacing),
                   incorrectColor,
                   QObject::tr("Incorrectas (%1)").arg(locale.toString(incorrect_)));
    }

private:
    int correct_ = 0;
    int incorrect_ = 0;
};

MainWindow::MainWindow(UserManager &userManager,
                       ProblemManager &problemManager,
                       QWidget *parent)
    : QMainWindow(parent),
      userManager_(userManager),
      problemManager_(problemManager) {
    setWindowTitle(tr("Proyecto PER"));
    resize(1200, 780);
    setMinimumSize(1024, 680);

    setupUi();
    applyAppTheme();
    updateSessionLabels();

    if (chartScene_) {
        connect(chartScene_, &ChartScene::textRequested, this, &MainWindow::handleTextRequested);
        connect(chartScene_, &ChartScene::distanceMeasured, this, &MainWindow::handleDistanceMeasured);
        connect(chartScene_, &ChartScene::statusMessage, this, &MainWindow::updateStatusMessage);
    }

    statusMessageTimer_.setSingleShot(true);
    connect(&statusMessageTimer_, &QTimer::timeout, this, [this]() {
        if (statusMessageLabel_) {
            statusMessageLabel_->clear();
            statusMessageLabel_->setVisible(false);
        }
    });

    stack_->setCurrentWidget(loginPage_);
    if (toolStrip_) {
        toolStrip_->setVisible(false);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    recordSessionIfNeeded();
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    applyProblemPaneConstraints();
}

void MainWindow::setToolFromAction(QAction *action) {
    if (!action) {
        return;
    }
    if (action == handAction_) {
        if (chartView_) {
            chartView_->setHandNavigationEnabled(true);
        }
        if (chartScene_) {
            chartScene_->setTool(ChartScene::Tool::Select);
        }
        return;
    }

    if (chartView_) {
        chartView_->setHandNavigationEnabled(false);
    }

    if (!chartScene_) {
        return;
    }

    if (action == crosshairAction_) {
        if (crosshairActive_) {
            crosshairActive_ = false;
            chartScene_->clearCrosshair();
            if (lastPrimaryToolAction_) {
                lastPrimaryToolAction_->setChecked(true);
            } else {
                chartScene_->setTool(ChartScene::Tool::Select);
            }
        } else {
            crosshairActive_ = true;
            chartScene_->setTool(ChartScene::Tool::Crosshair);
        }
        return;
    }

    crosshairActive_ = false;
    if (crosshairAction_ && crosshairAction_->isChecked()) {
        crosshairAction_->blockSignals(true);
        crosshairAction_->setChecked(false);
        crosshairAction_->blockSignals(false);
    }
    chartScene_->clearCrosshair();

    lastPrimaryToolAction_ = action;
    if (!action->data().isValid()) {
        chartScene_->setTool(ChartScene::Tool::Select);
        return;
    }
    const auto toolValue = action->data().toInt();
    chartScene_->setTool(static_cast<ChartScene::Tool>(toolValue));
}

void MainWindow::handleTextRequested(const QPointF &scenePos) {
    bool accepted = false;
    const QString text = QInputDialog::getMultiLineText(this, tr("Añadir anotación"), tr("Texto"), QString(), &accepted);
    if (accepted) {
        chartScene_->placeText(scenePos, text);
    } else {
        chartScene_->placeText(scenePos, QString());
    }
}

void MainWindow::handleDistanceMeasured(double pixels, double nauticalMiles) {
    showStatusBanner(tr("Medida: %1 px · %2 NM")
                         .arg(pixels, 0, 'f', 1)
                         .arg(nauticalMiles, 0, 'f', 2),
                     5000);
}

void MainWindow::updateStatusMessage(const QString &message) {
    showStatusBanner(message, 4000);
}

void MainWindow::handleColorActionTriggered(QAction *action) {
    if (!action) {
        return;
    }
    const QVariant data = action->data();
    if (!data.canConvert<QColor>()) {
        return;
    }

    const QColor color = data.value<QColor>();
    currentColorAction_ = action;
    action->setChecked(true);
    handleColorSelection(color);
}

void MainWindow::handleColorSelection(const QColor &color) {
    if (!color.isValid()) {
        return;
    }

    if (chartScene_) {
        chartScene_->setCurrentColor(color);
    }

    updateColorButtonIcon(color);
}

void MainWindow::updateColorButtonIcon(const QColor &color) {
    if (!colorButton_) {
        return;
    }

    const QSize iconSize(34, 22);
    QPixmap iconPixmap(iconSize.width() + 6, iconSize.height() + 6);
    iconPixmap.fill(Qt::transparent);

    QPainter painter(&iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRect swatchRect = iconPixmap.rect().adjusted(3, 3, -3, -3);
    painter.setBrush(color.isValid() ? color : QColor("#000000"));
    painter.setPen(QPen(QColor(0, 0, 0, 50), 1));
    painter.drawRoundedRect(swatchRect, 8, 8);
    painter.end();

    colorButton_->setIcon(QIcon(iconPixmap));
}

void MainWindow::loadProblemFromSelection(int index) {
    if (panelMode_ != QuestionPanelMode::Practice) {
        return;
    }
    if (!currentUser_ || !problemCombo_ || index < 0) {
        return;
    }
    const int id = problemCombo_->itemData(index).toInt();
    currentProblem_ = problemManager_.findById(id);
    updateAnswerOptions();
    resetAnswerSelection();
    updateProblemNavigationState();
}

void MainWindow::loadRandomProblem() {
    if (panelMode_ != QuestionPanelMode::Practice || !currentUser_) {
        return;
    }
    currentProblem_ = problemManager_.randomProblem();
    if (!currentProblem_) {
        problemStatement_->setPlainText(tr("No hay problemas disponibles."));
        resetAnswerSelection();
        return;
    }

    if (problemCombo_) {
        const int index = problemCombo_->findData(currentProblem_->id);
        if (index >= 0) {
            QSignalBlocker blocker(problemCombo_);
            problemCombo_->setCurrentIndex(index);
        }
    }
    updateAnswerOptions();
    resetAnswerSelection();
    updateProblemNavigationState();
}

void MainWindow::goToPreviousProblem() {
    if (panelMode_ == QuestionPanelMode::History) {
        if (currentHistoryIndex_ > 0) {
            --currentHistoryIndex_;
            updateHistoryDisplay();
        }
        return;
    }
    if (!problemCombo_ || problemCombo_->count() == 0) {
        return;
    }
    const int currentIndex = problemCombo_->currentIndex();
    if (currentIndex > 0) {
        problemCombo_->setCurrentIndex(currentIndex - 1);
    }
}

void MainWindow::goToNextProblem() {
    if (panelMode_ == QuestionPanelMode::History) {
        if (currentHistoryIndex_ >= 0 && currentHistoryIndex_ < historyAttempts_.size() - 1) {
            ++currentHistoryIndex_;
            updateHistoryDisplay();
        }
        return;
    }
    if (!problemCombo_ || problemCombo_->count() == 0) {
        return;
    }
    const int currentIndex = problemCombo_->currentIndex();
    if (currentIndex < problemCombo_->count() - 1) {
        problemCombo_->setCurrentIndex(currentIndex + 1);
    }
}

void MainWindow::toggleProblemPanel(bool collapsed) {
    problemPanelCollapsed_ = collapsed;
    if (problemBody_) {
        problemBody_->setVisible(!collapsed);
    }
    if (collapseProblemButton_) {
        collapseProblemButton_->setToolTip(collapsed ? tr("Mostrar panel") : tr("Ocultar panel"));
    }
    if (!contentSplitter_ || contentSplitter_->count() < 2) {
        updateToolStripLayout();
        return;
    }

    const QList<int> sizes = contentSplitter_->sizes();
    const int total = (sizes.size() >= 2) ? sizes.at(0) + sizes.at(1) : contentSplitter_->width();

    if (collapsed) {
        if (sizes.size() >= 2 && sizes.at(1) > 0) {
            lastProblemPaneWidth_ = sizes.at(1);
        }
        if (problemCard_) {
            problemCard_->setVisible(false);
        }
        QList<int> newSizes = sizes;
        if (newSizes.size() < 2) {
            newSizes = {total, 0};
        } else {
            newSizes[0] = total;
            newSizes[1] = 0;
        }
        contentSplitter_->setSizes(newSizes);
    } else {
        if (problemCard_) {
            problemCard_->setVisible(true);
            problemCard_->setMinimumWidth(kProblemPaneDefaultMinWidth);
            problemCard_->setMaximumWidth(QWIDGETSIZE_MAX);
        }
        const int available = total > 0 ? total : width();
        int desired = lastProblemPaneWidth_ > 0 ? lastProblemPaneWidth_ : available / 3;
        desired = clampProblemPaneWidth(available, desired);
        const int chartWidth = qMax(available - desired, 0);
        QList<int> restoredSizes;
        restoredSizes << chartWidth << desired;
        contentSplitter_->setSizes(restoredSizes);
        applyProblemPaneConstraints();
    }

    updateToolStripLayout();
}

void MainWindow::ensureProblemPaneVisible() {
    if (!problemPanelCollapsed_) {
        return;
    }
    if (collapseProblemButton_) {
        QSignalBlocker blocker(collapseProblemButton_);
        collapseProblemButton_->setChecked(false);
    }
    toggleProblemPanel(false);
}

void MainWindow::showStatusBanner(const QString &message, int timeoutMs) {
    if (!statusMessageLabel_) {
        return;
    }

    statusMessageLabel_->setText(message);
    statusMessageLabel_->setVisible(!message.isEmpty());

    statusMessageTimer_.stop();
    if (!message.isEmpty() && timeoutMs > 0) {
        statusMessageTimer_.start(timeoutMs);
    }
}

void MainWindow::updateProblemNavigationState() {
    if (panelMode_ == QuestionPanelMode::History) {
        updateHistoryNavigationState();
        return;
    }
    if (!problemCombo_) {
        if (prevProblemButton_) {
            prevProblemButton_->setEnabled(false);
        }
    }

    const int count = problemCombo_ ? problemCombo_->count() : 0;
    const int currentIndex = problemCombo_ ? problemCombo_->currentIndex() : -1;

    const bool hasPrev = count > 0 && currentIndex > 0;
    const bool hasNext = count > 0 && currentIndex >= 0 && currentIndex < count - 1;

    if (prevProblemButton_) {
        prevProblemButton_->setEnabled(hasPrev);
    }
    if (nextProblemButton_) {
        nextProblemButton_->setEnabled(hasNext);
    }
    if (problemBody_) {
        problemBody_->setEnabled(count > 0);
    }
}

void MainWindow::updateHistoryNavigationState() {
    const bool hasAttempts = !historyAttempts_.isEmpty();
    if (prevProblemButton_) {
        prevProblemButton_->setEnabled(hasAttempts && currentHistoryIndex_ > 0);
    }
    if (nextProblemButton_) {
        const bool canGoNext = hasAttempts && currentHistoryIndex_ >= 0 && currentHistoryIndex_ < historyAttempts_.size() - 1;
        nextProblemButton_->setEnabled(canGoNext);
    }
    if (problemBody_) {
        problemBody_->setEnabled(true);
    }
}

void MainWindow::setQuestionPanelMode(QuestionPanelMode mode) {
    if (statisticsViewActive_) {
        if (statisticsButton_) {
            QSignalBlocker blocker(statisticsButton_);
            statisticsButton_->setChecked(false);
        }
        showStatisticsView(false);
    }

    ensureProblemPaneVisible();
    panelMode_ = mode;
    const bool practice = panelMode_ == QuestionPanelMode::Practice;

    if (questionsToggleButton_) {
        QSignalBlocker blocker(questionsToggleButton_);
        questionsToggleButton_->setChecked(practice);
    }
    if (statsButton_) {
        QSignalBlocker blocker(statsButton_);
        statsButton_->setChecked(!practice);
    }

    if (navigationRow_) {
        navigationRow_->setVisible(practice);
    }
    if (problemCombo_) {
        problemCombo_->setVisible(practice);
    }
    if (randomButton_) {
        randomButton_->setVisible(practice);
    }

    if (historyControlsRow_) {
        historyControlsRow_->setVisible(!practice);
    }
    if (historySessionCombo_) {
        historySessionCombo_->setVisible(!practice);
    }

    if (historyStatusLabel_) {
        historyStatusLabel_->setVisible(!practice && !historyStatusLabel_->text().isEmpty());
    }

    if (submitButton_) {
        if (submitButtonDefaultText_.isEmpty()) {
            submitButtonDefaultText_ = submitButton_->text();
        }
        if (practice) {
            submitButton_->setText(submitButtonDefaultText_);
            submitButton_->setEnabled(answerButtons_ && answerButtons_->checkedButton());
        } else {
            submitButton_->setText(tr("Historial"));
            submitButton_->setEnabled(false);
        }
    }

    for (auto *option : answerOptions_) {
        option->setEnabled(practice);
        if (practice && option->isVisible()) {
            option->setStyleSheet(QString());
        }
    }

    if (practice) {
        updateHistoryStatusLabel();
        updateProblemNavigationState();
        if (problemCombo_) {
            loadProblemFromSelection(problemCombo_->currentIndex());
        } else if (currentProblem_) {
            updateAnswerOptions();
        }
    } else {
        refreshHistorySessionOptions();
        buildHistoryAttempts();
        updateHistoryDisplay();
    }
}

void MainWindow::showStatisticsView(bool active) {
    if (statisticsButton_) {
        QSignalBlocker blocker(statisticsButton_);
        statisticsButton_->setChecked(active);
    }

    if (!contentStack_ || !statisticsPage_ || !contentSplitter_) {
        return;
    }

    if (statisticsViewActive_ == active) {
        if (active) {
            updateStatisticsPanel();
        }
        return;
    }

    statisticsViewActive_ = active;

    if (active) {
        contentStack_->setCurrentWidget(statisticsPage_);
        if (toolStrip_) {
            toolStrip_->setVisible(false);
        }
        if (collapseProblemButton_) {
            collapseProblemButton_->setEnabled(false);
        }
        if (problemCard_) {
            problemCard_->setVisible(false);
        }
        updateStatisticsPanel();

        if (questionsToggleButton_) {
            QSignalBlocker blocker(questionsToggleButton_);
            questionsToggleButton_->setChecked(false);
        }
        if (statsButton_) {
            QSignalBlocker blocker(statsButton_);
            statsButton_->setChecked(false);
        }
        if (statsPieWidget_) {
            statsPieWidget_->setVisible(true);
        }
    } else {
        if (problemCard_) {
            problemCard_->setVisible(true);
        }
        if (collapseProblemButton_) {
            collapseProblemButton_->setEnabled(true);
        }
        if (toolStrip_) {
            toolStrip_->setVisible(true);
        }
        contentStack_->setCurrentWidget(contentSplitter_);

        const bool practice = panelMode_ == QuestionPanelMode::Practice;
        if (questionsToggleButton_) {
            QSignalBlocker blocker(questionsToggleButton_);
            questionsToggleButton_->setChecked(practice);
        }
        if (statsButton_) {
            QSignalBlocker blocker(statsButton_);
            statsButton_->setChecked(!practice);
        }
    }
}

void MainWindow::updateStatisticsPanel() {
    if (!statsTotalValueLabel_ || !statsCorrectValueLabel_ || !statsIncorrectValueLabel_ || !statsAccuracyValueLabel_) {
        return;
    }

    struct SessionStatsRow {
        QDateTime timestamp;
        int correct = 0;
        int incorrect = 0;
        bool isCurrent = false;
    };

    const auto hasAttempts = [](const SessionRecord &session) {
        return session.hits > 0 || session.faults > 0 || !session.attempts.isEmpty();
    };

    const auto computeRow = [&](const SessionRecord &session, bool current) {
        SessionStatsRow row;
        row.timestamp = session.timestamp;
        row.isCurrent = current;
        row.correct = session.hits;
        row.incorrect = session.faults;

        if (row.correct == 0 && row.incorrect == 0 && !session.attempts.isEmpty()) {
            for (const auto &attempt : session.attempts) {
                if (attempt.correct) {
                    ++row.correct;
                } else {
                    ++row.incorrect;
                }
            }
        }

        return row;
    };

    QVector<SessionStatsRow> rows;
    rows.reserve(currentUser_ ? currentUser_->sessions.size() + 1 : 1);

    if (hasAttempts(currentSession_)) {
        rows.push_back(computeRow(currentSession_, true));
    }

    QMap<QDate, SessionStatsRow> aggregatedByDate;
    if (currentUser_) {
        for (const auto &session : currentUser_->sessions) {
            if (!hasAttempts(session)) {
                continue;
            }
            SessionStatsRow row = computeRow(session, false);
            const QDate day = row.timestamp.date();
            auto it = aggregatedByDate.find(day);
            if (it == aggregatedByDate.end()) {
                aggregatedByDate.insert(day, row);
            } else {
                it->correct += row.correct;
                it->incorrect += row.incorrect;
                if (row.timestamp > it->timestamp) {
                    it->timestamp = row.timestamp;
                }
            }
        }
    }

    for (auto it = aggregatedByDate.begin(); it != aggregatedByDate.end(); ++it) {
        rows.push_back(it.value());
    }

    const auto totalFromRows = [](const SessionStatsRow &row) {
        return row.correct + row.incorrect;
    };

    QLocale locale;

    if (rows.isEmpty()) {
        statsTotalValueLabel_->setText(locale.toString(0));
        statsCorrectValueLabel_->setText(locale.toString(0));
        statsIncorrectValueLabel_->setText(locale.toString(0));
        statsAccuracyValueLabel_->setText(QStringLiteral("--"));

        if (statsTrendWidget_) {
            statsTrendWidget_->setBars({});
        }
        if (statsPieWidget_) {
            statsPieWidget_->setValues(0, 0);
        }
        if (statsSessionsTable_) {
            statsSessionsTable_->setRowCount(0);
        }
        if (statsSummaryCard_) {
            statsSummaryCard_->setVisible(false);
        }
        if (statsChartCard_) {
            statsChartCard_->setVisible(false);
        }
        if (statsTableCard_) {
            statsTableCard_->setVisible(false);
        }
        if (statsEmptyStateLabel_) {
            statsEmptyStateLabel_->setVisible(true);
        }
        return;
    }

    if (statsSummaryCard_) {
        statsSummaryCard_->setVisible(true);
    }
    if (statsChartCard_) {
        statsChartCard_->setVisible(true);
    }
    if (statsTableCard_) {
        statsTableCard_->setVisible(true);
    }
    if (statsEmptyStateLabel_) {
        statsEmptyStateLabel_->setVisible(false);
    }

    int totalCorrect = 0;
    int totalIncorrect = 0;
    for (const auto &row : rows) {
        totalCorrect += row.correct;
        totalIncorrect += row.incorrect;
    }

    const int totalAnswered = totalCorrect + totalIncorrect;
    const double accuracy = totalAnswered > 0 ? (static_cast<double>(totalCorrect) * 100.0) / totalAnswered : 0.0;

    statsTotalValueLabel_->setText(locale.toString(totalAnswered));
    statsCorrectValueLabel_->setText(locale.toString(totalCorrect));
    statsIncorrectValueLabel_->setText(locale.toString(totalIncorrect));
    statsAccuracyValueLabel_->setText(totalAnswered > 0 ? tr("%1 %").arg(locale.toString(accuracy, 'f', 1)) : QStringLiteral("--"));

    QVector<SessionStatsRow> chronologicalRows = rows;
    std::sort(chronologicalRows.begin(), chronologicalRows.end(), [](const SessionStatsRow &a, const SessionStatsRow &b) {
        return a.timestamp < b.timestamp;
    });

    QVector<StatsTrendWidget::BarData> chartBars;
    chartBars.reserve(qMin(kMaxStatsChartPoints, chronologicalRows.size()));
    const int startIndex = qMax(0, chronologicalRows.size() - kMaxStatsChartPoints);
    for (int idx = startIndex; idx < chronologicalRows.size(); ++idx) {
        const auto &row = chronologicalRows.at(idx);
        const int answered = totalFromRows(row);
        const double rowAccuracy = answered > 0 ? (static_cast<double>(row.correct) * 100.0) / answered : 0.0;
        StatsTrendWidget::BarData bar;
        if (row.timestamp.isValid()) {
            bar.label = row.timestamp.toString("dd/MM");
        } else {
            bar.label = QString::number(idx - startIndex + 1);
        }
        if (row.isCurrent) {
            bar.label = tr("Hoy");
        }
        bar.value = rowAccuracy;
        chartBars.push_back(std::move(bar));
    }
    if (statsTrendWidget_) {
        statsTrendWidget_->setBars(chartBars);
    }
    if (statsPieWidget_) {
        statsPieWidget_->setValues(totalCorrect, totalIncorrect);
    }

    std::sort(rows.begin(), rows.end(), [](const SessionStatsRow &a, const SessionStatsRow &b) {
        if (a.timestamp == b.timestamp) {
            return a.isCurrent && !b.isCurrent;
        }
        return a.timestamp > b.timestamp;
    });

    if (statsSessionsTable_) {
        const int displayRows = qMin<int>(kMaxStatsTableRows, rows.size());
        statsSessionsTable_->setRowCount(displayRows);

        for (int rowIndex = 0; rowIndex < displayRows; ++rowIndex) {
            const auto &row = rows.at(rowIndex);
            const int answered = totalFromRows(row);
            const double rowAccuracy = answered > 0 ? (static_cast<double>(row.correct) * 100.0) / answered : 0.0;

            const QString dateText = row.isCurrent && row.timestamp.isValid()
                                         ? tr("Sesión actual (%1)").arg(row.timestamp.toString("dd/MM hh:mm"))
                                         : row.timestamp.isValid() ? row.timestamp.toString("dd/MM/yyyy hh:mm")
                                                                   : tr("Sin fecha");

            auto *dateItem = new QTableWidgetItem(dateText);
            statsSessionsTable_->setItem(rowIndex, 0, dateItem);

            const auto makeNumericItem = [&](const QString &text) {
                auto *item = new QTableWidgetItem(text);
                item->setTextAlignment(Qt::AlignCenter);
                return item;
            };

            statsSessionsTable_->setItem(rowIndex, 1, makeNumericItem(locale.toString(answered)));
            statsSessionsTable_->setItem(rowIndex, 2, makeNumericItem(locale.toString(row.correct)));
            statsSessionsTable_->setItem(rowIndex, 3, makeNumericItem(locale.toString(row.incorrect)));
            statsSessionsTable_->setItem(rowIndex, 4, makeNumericItem(tr("%1 %").arg(locale.toString(rowAccuracy, 'f', 1))));
        }

        statsSessionsTable_->resizeRowsToContents();
    }
}

void MainWindow::buildHistoryAttempts() {
    historyAttempts_.clear();
    currentHistoryIndex_ = -1;

    const auto *source = selectedHistorySessionSource();
    if (!source || !source->attempts) {
        return;
    }

    historyAttempts_ = *(source->attempts);

    std::sort(historyAttempts_.begin(), historyAttempts_.end(), [](const QuestionAttempt &a, const QuestionAttempt &b) {
        return a.timestamp < b.timestamp;
    });

    if (!historyAttempts_.isEmpty()) {
        currentHistoryIndex_ = historyAttempts_.size() - 1;
    }
}

void MainWindow::updateHistoryStatusLabel(const QString &statusText) {
    if (!historyStatusLabel_) {
        return;
    }
    if (statusText.isEmpty()) {
        historyStatusLabel_->clear();
        historyStatusLabel_->setVisible(false);
    } else {
        historyStatusLabel_->setText(statusText);
        historyStatusLabel_->setVisible(true);
    }
}

void MainWindow::updateHistoryDisplay() {
    if (panelMode_ != QuestionPanelMode::History) {
        updateHistoryStatusLabel();
        return;
    }

    if (historyAttempts_.isEmpty() || currentHistoryIndex_ < 0) {
        if (problemStatement_) {
            problemStatement_->setPlainText(tr("No hay intentos registrados."));
        }
        for (auto *option : answerOptions_) {
            option->setVisible(false);
            option->setChecked(false);
            option->setStyleSheet(QString());
        }
        const QString message = historySessionSources_.isEmpty()
                                    ? tr("No hay sesiones registradas todavía.")
                                    : tr("No hay intentos para mostrar.");
        updateHistoryStatusLabel(message);
        updateHistoryNavigationState();
        return;
    }

    const int maxIndex = historyAttempts_.isEmpty() ? 0 : static_cast<int>(historyAttempts_.size() - 1);
    currentHistoryIndex_ = std::clamp(currentHistoryIndex_, 0, maxIndex);
    const auto &attempt = historyAttempts_.at(currentHistoryIndex_);
    const QString correctStyle = QString::fromLatin1(kCorrectAnswerStyle);
    const QString incorrectStyle = QString::fromLatin1(kIncorrectAnswerStyle);

    if (problemStatement_) {
        problemStatement_->setPlainText(attempt.question);
    }

    const QString statusText = tr("Intento %1 de %2 • %3")
                                   .arg(currentHistoryIndex_ + 1)
                                   .arg(historyAttempts_.size())
                                   .arg(attempt.correct ? tr("Correcto") : tr("Incorrecto"));
    updateHistoryStatusLabel(statusText);

    QVector<AttemptOption> options = attempt.options;
    if (options.isEmpty()) {
        if (!attempt.selectedAnswer.isEmpty()) {
            AttemptOption selected;
            selected.text = attempt.selectedAnswer;
            selected.correct = attempt.correct;
            options.push_back(selected);
        }
        if (!attempt.correctAnswer.isEmpty() && attempt.correctAnswer != attempt.selectedAnswer) {
            AttemptOption correct;
            correct.text = attempt.correctAnswer;
            correct.correct = true;
            options.push_back(correct);
        }
    }

    int selectedIndex = attempt.selectedIndex;
    if (selectedIndex < 0 || selectedIndex >= options.size()) {
        selectedIndex = !options.isEmpty() ? 0 : -1;
    }

    const int optionCount = qMin(options.size(), answerOptions_.size());
    for (int i = 0; i < answerOptions_.size(); ++i) {
        auto *option = answerOptions_.at(i);
        if (i < optionCount) {
            const auto &optionData = options.at(i);
            option->setVisible(true);
            option->setText(optionData.text);
            option->setProperty("valid", optionData.correct);
            option->setEnabled(false);
            option->setChecked(i == selectedIndex);
            if (optionData.correct) {
                option->setStyleSheet(correctStyle);
            } else if (i == selectedIndex) {
                option->setStyleSheet(incorrectStyle);
            } else {
                option->setStyleSheet(QString());
            }
        } else {
            option->setVisible(false);
            option->setChecked(false);
            option->setStyleSheet(QString());
        }
    }

    updateHistoryNavigationState();
}

const MainWindow::HistorySessionSource *MainWindow::selectedHistorySessionSource() const {
    if (historySessionSelection_ < 0 || historySessionSelection_ >= historySessionSources_.size()) {
        return nullptr;
    }
    return &historySessionSources_.at(historySessionSelection_);
}

void MainWindow::refreshHistorySessionOptions() {
    if (!historySessionCombo_) {
        historySessionSources_.clear();
        historySessionSelection_ = -1;
        return;
    }

    const QVector<QuestionAttempt> *previousSelection = nullptr;
    if (historySessionSelection_ >= 0 && historySessionSelection_ < historySessionSources_.size()) {
        previousSelection = historySessionSources_.at(historySessionSelection_).attempts;
    }

    historySessionSources_.clear();

    auto addSource = [this](HistorySessionSource source) {
        historySessionSources_.push_back(std::move(source));
    };

    if (!currentSession_.attempts.isEmpty()) {
        HistorySessionSource source;
        source.label = tr("Sesión actual (%1)").arg(currentSession_.timestamp.toString("dd/MM/yyyy hh:mm"));
        source.timestamp = currentSession_.timestamp;
        source.attempts = &currentSession_.attempts;
        source.isCurrentSession = true;
        addSource(std::move(source));
    }

    if (currentUser_) {
        for (const auto &session : currentUser_->sessions) {
            if (session.attempts.isEmpty()) {
                continue;
            }
            HistorySessionSource source;
            source.label = tr("%1 · %2 aciertos / %3 fallos")
                               .arg(session.timestamp.toString("dd/MM/yyyy hh:mm"))
                               .arg(session.hits)
                               .arg(session.faults);
            source.timestamp = session.timestamp;
            source.attempts = &session.attempts;
            source.isCurrentSession = false;
            addSource(std::move(source));
        }
    }

    std::sort(historySessionSources_.begin(), historySessionSources_.end(), [](const HistorySessionSource &lhs, const HistorySessionSource &rhs) {
        return lhs.timestamp > rhs.timestamp;
    });

    QSignalBlocker blocker(historySessionCombo_);
    historySessionCombo_->clear();
    for (const auto &source : historySessionSources_) {
        historySessionCombo_->addItem(source.label);
    }

    if (historySessionSources_.isEmpty()) {
        historySessionSelection_ = -1;
        historySessionCombo_->setEnabled(false);
        historyAttempts_.clear();
        currentHistoryIndex_ = -1;
        return;
    }

    int restoredIndex = -1;
    if (previousSelection) {
        for (int i = 0; i < historySessionSources_.size(); ++i) {
            if (historySessionSources_.at(i).attempts == previousSelection) {
                restoredIndex = i;
                break;
            }
        }
    }

    historySessionSelection_ = restoredIndex >= 0 ? restoredIndex : 0;
    historySessionCombo_->setEnabled(true);
    historySessionCombo_->setCurrentIndex(historySessionSelection_);
}

void MainWindow::handleHistorySessionSelectionChanged(int index) {
    historySessionSelection_ = index;
    buildHistoryAttempts();
    updateHistoryDisplay();
}

void MainWindow::handleSplitterMoved(int, int) {
    applyProblemPaneConstraints();
}

void MainWindow::applyProblemPaneConstraints(bool rememberWidth) {
    if (!contentSplitter_ || problemPanelCollapsed_) {
        return;
    }

    const QList<int> sizes = contentSplitter_->sizes();
    if (sizes.size() < 2) {
        return;
    }

    const int chartWidth = sizes.at(0);
    const int problemWidth = sizes.at(1);
    const int total = chartWidth + problemWidth;
    if (total <= 0) {
        return;
    }

    const int clamped = clampProblemPaneWidth(total, problemWidth > 0 ? problemWidth : lastProblemPaneWidth_);
    if (clamped <= 0) {
        return;
    }

    if (clamped != problemWidth) {
        QList<int> newSizes;
        newSizes << qMax(total - clamped, 0) << clamped;
        contentSplitter_->setSizes(newSizes);
    }

    if (rememberWidth) {
        lastProblemPaneWidth_ = clamped;
    }
}

int MainWindow::clampProblemPaneWidth(int totalWidth, int requestedWidth) const {
    if (totalWidth <= 0) {
        return requestedWidth;
    }

    constexpr double minRatio = 0.25;
    constexpr double maxRatio = 0.50;
    const int minWidth = qMax(1, static_cast<int>(std::ceil(totalWidth * minRatio)));
    const int maxWidth = qMax(minWidth, static_cast<int>(std::floor(totalWidth * maxRatio)));
    const int fallback = requestedWidth > 0 ? requestedWidth : maxWidth;
    return std::clamp(fallback, minWidth, maxWidth);
}

void MainWindow::submitAnswer() {
    if (!currentUser_ || !currentProblem_ || !answerButtons_) {
        return;
    }

    const int checkedId = answerButtons_->checkedId();
    if (checkedId < 0 || checkedId >= answerOptions_.size()) {
        QMessageBox::information(this, tr("Respuesta"), tr("Selecciona una respuesta antes de comprobar."));
        return;
    }

    auto *button = answerOptions_.at(checkedId);
    const bool isCorrect = button->property("valid").toBool();
    const QString correctStyle = QString::fromLatin1(kCorrectAnswerStyle);
    const QString incorrectStyle = QString::fromLatin1(kIncorrectAnswerStyle);
    if (isCorrect) {
        currentSession_.hits += 1;
        button->setStyleSheet(correctStyle);
        updateStatusMessage(tr("¡Correcto!"));
    } else {
        currentSession_.faults += 1;
        button->setStyleSheet(incorrectStyle);
        for (auto *candidate : answerOptions_) {
            if (candidate->property("valid").toBool()) {
                candidate->setStyleSheet(correctStyle);
            }
        }
        updateStatusMessage(tr("Respuesta incorrecta."));
    }

    submitButton_->setEnabled(false);
    if (currentProblem_) {
        QuestionAttempt attempt;
        attempt.timestamp = QDateTime::currentDateTime();
        attempt.problemId = currentProblem_->id;
        attempt.question = currentProblem_->text;
        attempt.selectedAnswer = button->text();
        attempt.correct = isCorrect;

        attempt.options.reserve(answerOptions_.size());
        for (auto *candidate : answerOptions_) {
            if (!candidate->isVisible()) {
                continue;
            }
            AttemptOption option;
            option.text = candidate->text();
            option.correct = candidate->property("valid").toBool();
            attempt.options.push_back(option);
            if (candidate == button) {
                attempt.selectedIndex = attempt.options.size() - 1;
            }
            if (option.correct && attempt.correctAnswer.isEmpty()) {
                attempt.correctAnswer = candidate->text();
            }
        }
        if (attempt.correctAnswer.isEmpty()) {
            attempt.correctAnswer = button->text();
        }
        currentSession_.attempts.push_back(std::move(attempt));
    }
    updateSessionLabels();
}

void MainWindow::showProfileDialog() {
    if (!currentUser_ || guestSessionActive_) {
        return;
    }
    ProfileDialog dialog(userManager_, *currentUser_, this);
    if (dialog.exec() == QDialog::Accepted) {
        currentUser_ = dialog.updatedUser();
        updateUserPanel();
    }
}

void MainWindow::showResultsDialog() {
    if (!currentUser_ || guestSessionActive_) {
        return;
    }
    ResultsDialog dialog(currentUser_->sessions, this);
    dialog.exec();
}

void MainWindow::logout() {
    recordSessionIfNeeded();
    updateStatusMessage(tr("Sesión cerrada"));
    returnToLogin();
}

void MainWindow::toggleProtractor(bool checked) {
    if (chartScene_) {
        chartScene_->setProtractorVisible(checked);
    }
}

void MainWindow::toggleRuler(bool checked) {
    if (chartScene_) {
        chartScene_->setRulerVisible(checked);
    }
}

void MainWindow::toggleFullscreenMode(bool checked) {
    fullScreenModeActive_ = checked;
    if (fullScreenAction_) {
        fullScreenAction_->setToolTip(checked ? tr("Salir de pantalla completa") : tr("Mostrar la carta sin distracciones"));
    }

    if (checked) {
        questionPanelVisibleBeforeFullscreen_ = !problemPanelCollapsed_;
        questionPanelModeBeforeFullscreen_ = panelMode_;
        topBarVisibleBeforeFullscreen_ = topBar_ ? topBar_->isVisible() : true;

        if (!problemPanelCollapsed_) {
            if (collapseProblemButton_) {
                QSignalBlocker blocker(collapseProblemButton_);
                collapseProblemButton_->setChecked(true);
            }
            toggleProblemPanel(true);
        }
        if (topBar_) {
            topBar_->setVisible(false);
        }
    } else {
        if (topBar_) {
            topBar_->setVisible(topBarVisibleBeforeFullscreen_);
        }
        if (questionPanelVisibleBeforeFullscreen_) {
            setQuestionPanelMode(questionPanelModeBeforeFullscreen_);
        }
    }
}

void MainWindow::toggleExtremes() {
    if (chartScene_) {
        chartScene_->toggleExtremesForSelection();
    }
}

void MainWindow::clearChart() {
    if (chartScene_) {
        chartScene_->clearMarks();
    }
}

void MainWindow::zoomInOnChart() {
    if (chartView_) {
        chartView_->zoomIn();
    }
}

void MainWindow::zoomOutOnChart() {
    if (chartView_) {
        chartView_->zoomOut();
    }
}

void MainWindow::resetChartZoom() {
    if (chartView_) {
        chartView_->resetZoom();
    }
}

void MainWindow::attemptLogin() {
    if (!loginButton_) {
        return;
    }

    const QString username = loginUserEdit_->text().trimmed();
    const QString password = loginPasswordEdit_->text();

    if (username.isEmpty() || password.isEmpty()) {
        return;
    }

    QString error;
    const auto authenticated = userManager_.authenticate(username, password, error);
    if (!authenticated) {
        loginFeedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
        loginFeedbackLabel_->setText(error);
        loginFeedbackLabel_->setVisible(true);
        return;
    }

    loginFeedbackLabel_->clear();
    loginFeedbackLabel_->setVisible(false);
    enterApplication(*authenticated);
}

void MainWindow::startGuestSession() {
    UserRecord guest;
    guest.nickname = tr("Invitado");
    guest.email = tr("Sesión temporal");
    guest.avatarPath = QString::fromLatin1(kDefaultAvatarPath);

    if (loginFeedbackLabel_) {
        loginFeedbackLabel_->clear();
        loginFeedbackLabel_->setVisible(false);
        loginFeedbackLabel_->setStyleSheet(QString());
    }
    if (loginUserEdit_) {
        loginUserEdit_->clear();
    }
    if (loginPasswordEdit_) {
        loginPasswordEdit_->clear();
    }

    enterApplication(guest, true);
}

void MainWindow::validateLoginForm() {
    if (!loginButton_) {
        return;
    }

    const bool ready = !loginUserEdit_->text().trimmed().isEmpty() && !loginPasswordEdit_->text().isEmpty();
    loginButton_->setEnabled(ready);
    if (!ready) {
        if (loginFeedbackLabel_) {
            loginFeedbackLabel_->clear();
            loginFeedbackLabel_->setVisible(false);
            loginFeedbackLabel_->setStyleSheet(QString());
        }
    }
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stack_ = new QStackedWidget(central);
    loginPage_ = createLoginPage();
    registerPage_ = createRegisterPage();
    appPage_ = createAppPage();
    stack_->addWidget(loginPage_);
    stack_->addWidget(registerPage_);  
    stack_->addWidget(appPage_);

    layout->addWidget(stack_);
    setCentralWidget(central);
}

QWidget *MainWindow::createLoginPage() {
    auto *page = new QWidget(this);
    page->setObjectName("LoginPage");
    auto *outerLayout = new QVBoxLayout(page);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto *centerWrapper = new QWidget(page);
    centerWrapper->setObjectName("LoginCenter");
    auto *centerLayout = new QVBoxLayout(centerWrapper);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    centerLayout->setAlignment(Qt::AlignCenter);

    auto *card = new QFrame(centerWrapper);
    card->setObjectName("LoginCard");
    card->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 24, 22, 26);
    cardLayout->setSpacing(14);

    loginFormPage_ = buildLoginFormPage(card);
    cardLayout->addWidget(loginFormPage_);

    centerLayout->addWidget(card, 0, Qt::AlignCenter);
    outerLayout->addWidget(centerWrapper, 1);

    return page;
}

// --------------- Registration Page ----------------
QWidget *MainWindow::createRegisterPage() {
    auto *page = new QWidget(this);
    page->setObjectName("RegistrationPage");
    auto *outerLayout = new QVBoxLayout(page);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto *centerWrapper = new QWidget(page);
    centerWrapper->setObjectName("RegisterCenter");
    auto *centerLayout = new QVBoxLayout(centerWrapper);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    centerLayout->setAlignment(Qt::AlignCenter);

    auto *card = new QFrame(centerWrapper);
    card->setObjectName("RegisterCard");
    card->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 24, 22, 26);
    cardLayout->setSpacing(14);
    
    registerFormPage_ = buildRegisterFormPage(card);
    cardLayout->addWidget(registerFormPage_);

    centerLayout->addWidget(card, 0, Qt::AlignCenter);
    outerLayout->addWidget(centerWrapper, 1);

    return page;
}

// ------------------------------------------------



QWidget *MainWindow::buildLoginFormPage(QWidget *parent) {
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("Bienvenido de nuevo"), page);
    title->setObjectName("LoginTitle");

    auto *subtitle = new QLabel(tr("Inicia sesión para continuar navegando por la carta."), page);
    subtitle->setObjectName("LoginSubtitle");
    subtitle->setWordWrap(true);

    loginUserEdit_ = new QLineEdit(page);
    loginUserEdit_->setPlaceholderText(tr("Usuario"));
    loginUserEdit_->setClearButtonEnabled(true);

    loginPasswordEdit_ = new QLineEdit(page);
    loginPasswordEdit_->setEchoMode(QLineEdit::Password);
    loginPasswordEdit_->setPlaceholderText(tr("Contraseña"));
    loginPasswordEdit_->setClearButtonEnabled(true);

    loginFeedbackLabel_ = new QLabel(page);
    loginFeedbackLabel_->setObjectName("LoginFeedback");
    loginFeedbackLabel_->setWordWrap(true);
    loginFeedbackLabel_->setVisible(false);

    loginButton_ = new QPushButton(tr("Entrar"), page);
    loginButton_->setObjectName("LoginButton");
    loginButton_->setEnabled(false);

    guestLoginButton_ = new QPushButton(tr("Acceso de Invitado"), page);
    guestLoginButton_->setObjectName("GuestLoginButton");
    guestLoginButton_->setCursor(Qt::PointingHandCursor);
    guestLoginButton_->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "background-color: #ffffff;"
        "color: #0b3d70;"
        "border: 1px solid #0b3d70;"
        "border-radius: 8px;"
        "padding: 8px 14px;"
        "font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "background-color: #f2f6ff;"
        "}"
        "QPushButton:pressed {"
        "background-color: #e3ecff;"
        "}"
    ));

    auto *registerRow = new QHBoxLayout();
    registerRow->setContentsMargins(0, 0, 0, 0);
    registerRow->setSpacing(8);
    auto *registerHint = new QLabel(tr("¿Aún no tienes cuenta?"), page);
    registerHint->setObjectName("RegisterHint");
    auto *registerButton = new QPushButton(tr("Crear cuenta"), page);
    registerButton->setObjectName("RegisterButton");
    registerButton->setCursor(Qt::PointingHandCursor);
    registerRow->addWidget(registerHint);
    registerRow->addWidget(registerButton);
    registerRow->addStretch(1);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(6);
    layout->addWidget(loginUserEdit_);
    layout->addWidget(loginPasswordEdit_);
    layout->addWidget(loginFeedbackLabel_);
    layout->addWidget(loginButton_);
    layout->addWidget(guestLoginButton_);
    layout->addLayout(registerRow);

    connect(loginUserEdit_, &QLineEdit::textChanged, this, &MainWindow::validateLoginForm);
    connect(loginPasswordEdit_, &QLineEdit::textChanged, this, &MainWindow::validateLoginForm);
    connect(loginPasswordEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptLogin);
    connect(loginButton_, &QPushButton::clicked, this, &MainWindow::attemptLogin);
    connect(guestLoginButton_, &QPushButton::clicked, this, &MainWindow::startGuestSession);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::showRegistrationForm);

    return page;
}

QWidget *MainWindow::buildRegisterFormPage(QWidget *parent) {
    auto *page = new QWidget(parent);
    page->setObjectName("RegisterPage");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Crear cuenta"), page);
    title->setObjectName("RegisterTitle");

    auto *subtitle = new QLabel(tr("Configura tu cuenta para comenzar a practicar."), page);
    subtitle->setObjectName("RegisterSubtitle");
    subtitle->setWordWrap(true);

    auto *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);

    registerNicknameEdit_ = new QLineEdit(page);
    registerNicknameEdit_->setPlaceholderText(tr("Entre 6 y 15 caracteres"));

    registerEmailEdit_ = new QLineEdit(page);
    registerEmailEdit_->setPlaceholderText(tr("ejemplo@correo.com"));

    registerPasswordEdit_ = new QLineEdit(page);
    registerPasswordEdit_->setEchoMode(QLineEdit::Password);

    registerConfirmPasswordEdit_ = new QLineEdit(page);
    registerConfirmPasswordEdit_->setEchoMode(QLineEdit::Password);

    registerBirthdateEdit_ = new QDateEdit(QDate::currentDate().addYears(-18), page);
    registerBirthdateEdit_->setCalendarPopup(true);
    registerBirthdateEdit_->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    registerAvatarPreview_ = new QLabel(page);
    registerAvatarPreview_->setObjectName("AvatarPreview");
    registerAvatarPreview_->setFixedSize(kAvatarPreviewSize, kAvatarPreviewSize);
    registerAvatarPreview_->setPixmap(QPixmap(kDefaultAvatarPath).scaled(kAvatarPreviewSize, kAvatarPreviewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *avatarButton = new QPushButton(tr("Seleccionar avatar"), page);
    avatarButton->setObjectName("AvatarButton");

    auto *avatarLayout = new QHBoxLayout();
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(12);
    avatarLayout->addWidget(registerAvatarPreview_);
    avatarLayout->addWidget(avatarButton);
    avatarLayout->addStretch(1);

    formLayout->addRow(tr("Usuario"), registerNicknameEdit_);
    formLayout->addRow(tr("Correo electrónico"), registerEmailEdit_);
    formLayout->addRow(tr("Contraseña"), registerPasswordEdit_);
    formLayout->addRow(tr("Confirmar contraseña"), registerConfirmPasswordEdit_);
    formLayout->addRow(tr("Fecha de nacimiento"), registerBirthdateEdit_);
    formLayout->addRow(tr("Avatar"), avatarLayout);

    registerFeedbackLabel_ = new QLabel(page);
    registerFeedbackLabel_->setObjectName("RegisterFeedback");
    registerFeedbackLabel_->setWordWrap(true);
    registerFeedbackLabel_->setVisible(false);

    registerSubmitButton_ = new QPushButton(tr("Crear cuenta"), page);
    registerSubmitButton_->setObjectName("RegisterSubmitButton");
    registerSubmitButton_->setEnabled(false);

    auto *backButton = new QPushButton(tr("Ya tengo cuenta"), page);
    backButton->setObjectName("BackToLoginButton");

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addLayout(formLayout);
    layout->addWidget(registerFeedbackLabel_);
    layout->addWidget(registerSubmitButton_);
    layout->addWidget(backButton);

    const auto triggerValidation = [this]() {
        validateRegisterForm();
    };

    connect(registerNicknameEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(registerEmailEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(registerPasswordEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(registerConfirmPasswordEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(registerBirthdateEdit_, &QDateEdit::dateChanged, this, [this](const QDate &) { validateRegisterForm(); });
    connect(avatarButton, &QPushButton::clicked, this, &MainWindow::selectRegisterAvatar);
    connect(registerSubmitButton_, &QPushButton::clicked, this, &MainWindow::handleRegisterSubmit);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showLoginForm);

    return page;
}

void MainWindow::showRegistrationForm() {
    if (!stack_ || !registerPage_ || !registerFormPage_) {
        return;
    }
    resetRegisterForm();
    if (loginFeedbackLabel_) {
        loginFeedbackLabel_->clear();
        loginFeedbackLabel_->setVisible(false);
        loginFeedbackLabel_->setStyleSheet(QString());
    }
    stack_->setCurrentWidget(registerPage_);
    if (registerNicknameEdit_) {
        registerNicknameEdit_->setFocus();
    }
}

void MainWindow::showLoginForm() {
    if (!stack_ || !loginPage_ || !loginFormPage_) {
        return;
    }
    stack_->setCurrentWidget(loginPage_);
    if (loginFeedbackLabel_) {
        loginFeedbackLabel_->clear();
        loginFeedbackLabel_->setVisible(false);
        loginFeedbackLabel_->setStyleSheet(QString());
    }
    if (loginUserEdit_) {
        loginUserEdit_->setFocus();
        loginUserEdit_->setCursorPosition(loginUserEdit_->text().size());
    }
}

void MainWindow::selectRegisterAvatar() {
    const QString file = QFileDialog::getOpenFileName(this,
                                                      tr("Seleccionar avatar"),
                                                      QString(),
                                                      tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.svg)"));
    if (file.isEmpty()) {
        return;
    }

    registerAvatarPath_ = file;
    if (registerAvatarPreview_) {
        QPixmap pixmap(file);
        registerAvatarPreview_->setPixmap(pixmap.scaled(kAvatarPreviewSize,
                                                        kAvatarPreviewSize,
                                                        Qt::KeepAspectRatio,
                                                        Qt::SmoothTransformation));
    }
    validateRegisterForm();
}

void MainWindow::validateRegisterForm() {
    if (!registerSubmitButton_ || !registerNicknameEdit_ || !registerEmailEdit_ ||
        !registerPasswordEdit_ || !registerConfirmPasswordEdit_) {
        return;
    }

    const bool allFilled = !registerNicknameEdit_->text().trimmed().isEmpty() &&
                           !registerEmailEdit_->text().trimmed().isEmpty() &&
                           !registerPasswordEdit_->text().isEmpty() &&
                           !registerConfirmPasswordEdit_->text().isEmpty();

    if (!allFilled) {
        registerSubmitButton_->setEnabled(false);
        if (registerFeedbackLabel_) {
            registerFeedbackLabel_->setVisible(false);
            registerFeedbackLabel_->clear();
            registerFeedbackLabel_->setStyleSheet(QString());
        }
        return;
    }

    QString error;
    const bool valid = validateRegisterInputs(error);
    registerSubmitButton_->setEnabled(valid);
    if (registerFeedbackLabel_) {
        if (error.isEmpty()) {
            registerFeedbackLabel_->clear();
            registerFeedbackLabel_->setVisible(false);
            registerFeedbackLabel_->setStyleSheet(QString());
        } else {
            registerFeedbackLabel_->setText(error);
            registerFeedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
            registerFeedbackLabel_->setVisible(true);
        }
    }
}

void MainWindow::handleRegisterSubmit() {
    if (!registerNicknameEdit_ || !registerEmailEdit_ || !registerPasswordEdit_ ||
        !registerConfirmPasswordEdit_ || !registerBirthdateEdit_) {
        return;
    }

    QString validationError;
    if (!validateRegisterInputs(validationError)) {
        if (registerFeedbackLabel_) {
            registerFeedbackLabel_->setText(validationError);
            registerFeedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
            registerFeedbackLabel_->setVisible(true);
        }
        return;
    }

    const QString nickname = registerNicknameEdit_->text().trimmed();
    const QString email = registerEmailEdit_->text().trimmed();
    const QString password = registerPasswordEdit_->text();
    const QDate birthdate = registerBirthdateEdit_->date();

    QString error;
    if (!userManager_.registerUser(nickname, email, password, birthdate, registerAvatarPath_, error)) {
        if (registerFeedbackLabel_) {
            registerFeedbackLabel_->setText(error);
            registerFeedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
            registerFeedbackLabel_->setVisible(true);
        }
        return;
    }

    if (registerFeedbackLabel_) {
        registerFeedbackLabel_->clear();
        registerFeedbackLabel_->setVisible(false);
        registerFeedbackLabel_->setStyleSheet(QString());
    }

    resetRegisterForm();
    showLoginForm();

    if (loginUserEdit_) {
        loginUserEdit_->setText(nickname);
    }
    if (loginPasswordEdit_) {
        loginPasswordEdit_->clear();
        loginPasswordEdit_->setFocus();
    }
    if (loginButton_) {
        loginButton_->setEnabled(false);
    }
    if (loginFeedbackLabel_) {
        loginFeedbackLabel_->setStyleSheet(QStringLiteral("color: #1f7a4d;"));
        loginFeedbackLabel_->setText(tr("Cuenta creada. Inicia sesión con tus credenciales."));
        loginFeedbackLabel_->setVisible(true);
    }
}

void MainWindow::resetRegisterForm() {
    registerAvatarPath_.clear();

    if (registerNicknameEdit_) {
        registerNicknameEdit_->clear();
    }
    if (registerEmailEdit_) {
        registerEmailEdit_->clear();
    }
    if (registerPasswordEdit_) {
        registerPasswordEdit_->clear();
    }
    if (registerConfirmPasswordEdit_) {
        registerConfirmPasswordEdit_->clear();
    }
    if (registerBirthdateEdit_) {
        registerBirthdateEdit_->setDate(QDate::currentDate().addYears(-18));
    }
    if (registerAvatarPreview_) {
        registerAvatarPreview_->setPixmap(QPixmap(kDefaultAvatarPath).scaled(kAvatarPreviewSize,
                                                                            kAvatarPreviewSize,
                                                                            Qt::KeepAspectRatio,
                                                                            Qt::SmoothTransformation));
    }
    if (registerFeedbackLabel_) {
        registerFeedbackLabel_->clear();
        registerFeedbackLabel_->setVisible(false);
        registerFeedbackLabel_->setStyleSheet(QString());
    }
    if (registerSubmitButton_) {
        registerSubmitButton_->setEnabled(false);
    }
    validateRegisterForm();
}

bool MainWindow::validateRegisterInputs(QString &errorMessage) const {
    if (!registerNicknameEdit_ || !registerEmailEdit_ || !registerPasswordEdit_ ||
        !registerConfirmPasswordEdit_ || !registerBirthdateEdit_) {
        errorMessage = tr("Completa todos los campos.");
        return false;
    }

    const QString nickname = registerNicknameEdit_->text().trimmed();
    const QString email = registerEmailEdit_->text().trimmed();
    const QString password = registerPasswordEdit_->text();
    const QString confirmPassword = registerConfirmPasswordEdit_->text();
    const QDate birthdate = registerBirthdateEdit_->date();

    if (nickname.size() < 6 || nickname.size() > 15) {
        errorMessage = tr("El usuario debe tener entre 6 y 15 caracteres.");
        return false;
    }

    static const QRegularExpression nicknameRegex(QStringLiteral("^[A-Za-z0-9_-]+$"));
    if (!nicknameRegex.match(nickname).hasMatch()) {
        errorMessage = tr("El usuario solo puede contener letras, números, guiones y guiones bajos.");
        return false;
    }

    static const QRegularExpression emailRegex(QStringLiteral("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"));
    if (!emailRegex.match(email).hasMatch()) {
        errorMessage = tr("Correo electrónico no válido.");
        return false;
    }

    if (password.size() < 8 || password.size() > 20) {
        errorMessage = tr("La contraseña debe tener entre 8 y 20 caracteres.");
        return false;
    }

    static const QRegularExpression upperRegex(QStringLiteral("[A-Z]"));
    static const QRegularExpression lowerRegex(QStringLiteral("[a-z]"));
    static const QRegularExpression digitRegex(QStringLiteral("[0-9]"));
    static const QRegularExpression specialRegex(QStringLiteral("[-!@#$%&*()+=]"));

    if (!upperRegex.match(password).hasMatch() ||
        !lowerRegex.match(password).hasMatch() ||
        !digitRegex.match(password).hasMatch() ||
        !specialRegex.match(password).hasMatch()) {
        errorMessage = tr("La contraseña debe incluir mayúsculas, minúsculas, dígitos y caracteres especiales.");
        return false;
    }

    if (password != confirmPassword) {
        errorMessage = tr("Las contraseñas no coinciden.");
        return false;
    }

    const int age = birthdate.daysTo(QDate::currentDate()) / 365;
    if (age < 16) {
        errorMessage = tr("Debes ser mayor de 16 años.");
        return false;
    }

    errorMessage.clear();
    return true;
}

QWidget *MainWindow::createAppPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    // Chart must exist before building tool strip
    chartScene_ = new ChartScene(this);
    chartScene_->setBackgroundPixmap(QPixmap(":/resources/images/carta_nautica.png"));

    chartView_ = new ChartView(this);
    chartView_->setScene(chartScene_);
    chartView_->setHandNavigationEnabled(true);
    chartView_->setFrameShape(QFrame::NoFrame);
    chartView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chartView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *topBar = new QFrame(page);
    topBar_ = topBar;
    topBar->setObjectName("TopBar");
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(24, 16, 24, 16);
    topLayout->setSpacing(16);

    auto *title = new QLabel(tr("Proyecto PER"), topBar);
    title->setObjectName("AppTitle");

    userSummaryLabel_ = new QLabel(tr("Sin sesión activa"), topBar);
    userSummaryLabel_->setObjectName("UserSummary");

    questionsToggleButton_ = new QToolButton(topBar);
    questionsToggleButton_->setObjectName("QuestionsToggleButton");
    questionsToggleButton_->setText(tr("Preguntas"));
    questionsToggleButton_->setCheckable(true);
    questionsToggleButton_->setChecked(true);
    questionsToggleButton_->setAutoRaise(true);
    questionsToggleButton_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    questionsToggleButton_->setEnabled(false);

    statsButton_ = new QToolButton(topBar);
    statsButton_->setObjectName("StatsButton");
    statsButton_->setText(tr("Histórico"));
    statsButton_->setAutoRaise(true);
    statsButton_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    statsButton_->setCheckable(true);
    statsButton_->setEnabled(false);

    auto *historyStatsSeparator = new QLabel(tr("|"), topBar);
    historyStatsSeparator->setObjectName("HistoryStatsSeparator");
    historyStatsSeparator->setStyleSheet(QStringLiteral("color: #6e7781;"));

    statisticsButton_ = new QToolButton(topBar);
    statisticsButton_->setObjectName("StatisticsButton");
    statisticsButton_->setText(tr("Estadísticas"));
    statisticsButton_->setAutoRaise(true);
    statisticsButton_->setToolButtonStyle(Qt::ToolButtonTextOnly);
    statisticsButton_->setCheckable(true);
    statisticsButton_->setEnabled(false);

    sessionStatsLabel_ = new QLabel(tr("Aciertos: 0 · Fallos: 0"), topBar);
    sessionStatsLabel_->setObjectName("SessionStats");

    statusMessageLabel_ = new QLabel(topBar);
    statusMessageLabel_->setObjectName("StatusMessageLabel");
    statusMessageLabel_->setVisible(false);
    statusMessageLabel_->setStyleSheet(QStringLiteral("color: #6e7781;"));

    userMenuButton_ = new QToolButton(topBar);
    userMenuButton_->setObjectName("UserButton");
    userMenuButton_->setAutoRaise(true);
    userMenuButton_->setPopupMode(QToolButton::InstantPopup);
    userMenuButton_->setIconSize(QSize(kAvatarIconSize, kAvatarIconSize));
    userMenuButton_->setIcon(QIcon(makeCircularAvatar(kDefaultAvatarPath)));

    userMenu_ = new QMenu(userMenuButton_);
    profileAction_ = userMenu_->addAction(QIcon(":/resources/images/icon_profile.svg"), tr("Editar perfil"));
    resultsAction_ = userMenu_->addAction(QIcon(":/resources/images/icon_results.svg"), tr("Historial de sesiones"));
    userMenu_->addSeparator();
    logoutAction_ = userMenu_->addAction(QIcon(":/resources/images/icon_logout.svg"), tr("Cerrar sesión"));
    profileAction_->setIconVisibleInMenu(true);
    resultsAction_->setIconVisibleInMenu(true);
    logoutAction_->setIconVisibleInMenu(true);
    userMenuButton_->setMenu(userMenu_);

    topLayout->addWidget(title);
    topLayout->addSpacing(12);
    topLayout->addWidget(userSummaryLabel_);
    topLayout->addWidget(questionsToggleButton_);
    topLayout->addWidget(statsButton_);
    topLayout->addWidget(historyStatsSeparator);
    topLayout->addWidget(statisticsButton_);
    topLayout->addStretch(1);
    topLayout->addWidget(sessionStatsLabel_);
    topLayout->addWidget(statusMessageLabel_);
    topLayout->addWidget(userMenuButton_);

    layout->addWidget(topBar);

    auto *chartCard = new QFrame(page);
    chartCard->setObjectName("ChartCard");
    chartCard->setMinimumWidth(520);
    auto *chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setContentsMargins(16, 16, 16, 16);
    chartLayout->setSpacing(12);

    toolStrip_ = createToolStrip(chartCard);
    chartLayout->addWidget(toolStrip_);
    chartLayout->addWidget(chartView_, 1);

    problemCard_ = new QFrame(page);
    problemCard_->setObjectName("ProblemCard");
    problemCard_->setMinimumWidth(kProblemPaneDefaultMinWidth);
    auto *problemLayout = new QVBoxLayout(problemCard_);
    problemLayout->setContentsMargins(16, 16, 16, 16);
    problemLayout->setSpacing(12);

    auto *problemHeader = new QHBoxLayout();
    problemHeader->setContentsMargins(0, 0, 0, 0);
    problemHeader->setSpacing(8);

    auto *problemTitle = new QLabel(tr("Problemas de examen"), problemCard_);
    problemTitle->setObjectName("ProblemTitle");

    collapseProblemButton_ = new QToolButton(problemCard_);
    collapseProblemButton_->setObjectName("ProblemCollapseButton");
    collapseProblemButton_->setCheckable(true);
    collapseProblemButton_->setChecked(false);
    collapseProblemButton_->setIcon(QIcon(":/resources/images/icon_cross.svg"));
    collapseProblemButton_->setIconSize(QSize(26, 26));
    collapseProblemButton_->setToolTip(tr("Ocultar panel"));

    problemHeader->addWidget(problemTitle);
    problemHeader->addStretch(1);
    problemHeader->addWidget(collapseProblemButton_);

    problemBody_ = new QWidget(problemCard_);
    auto *bodyLayout = new QVBoxLayout(problemBody_);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(10);

    navigationRow_ = new QWidget(problemBody_);
    navigationRow_->setObjectName("ProblemNavigationRow");
    auto *navigationLayout = new QHBoxLayout(navigationRow_);
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(8);

    prevProblemButton_ = new QPushButton(problemBody_);
    prevProblemButton_->setObjectName("PrevProblemButton");
    prevProblemButton_->setFlat(false);
    prevProblemButton_->setText(QString());
    prevProblemButton_->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    prevProblemButton_->setIconSize(QSize(20, 20));
    prevProblemButton_->setToolTip(tr("Pregunta anterior"));
    prevProblemButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    problemCombo_ = new QComboBox(problemBody_);
    problemCombo_->setObjectName("ProblemSelector");

    nextProblemButton_ = new QPushButton(problemBody_);
    nextProblemButton_->setObjectName("NextProblemButton");
    nextProblemButton_->setFlat(false);
    nextProblemButton_->setText(QString());
    nextProblemButton_->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    nextProblemButton_->setIconSize(QSize(20, 20));
    nextProblemButton_->setToolTip(tr("Pregunta siguiente"));
    nextProblemButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    randomButton_ = new QPushButton(tr("Aleatorio"), problemBody_);
    randomButton_->setObjectName("RandomButton");

    navigationLayout->addWidget(problemCombo_, 1);
    navigationLayout->addWidget(randomButton_);

    historyControlsRow_ = new QWidget(problemBody_);
    historyControlsRow_->setObjectName("HistoryControlsRow");
    auto *historyControlsLayout = new QHBoxLayout(historyControlsRow_);
    historyControlsLayout->setContentsMargins(0, 0, 0, 0);
    historyControlsLayout->setSpacing(8);

    auto *historySessionLabel = new QLabel(tr("Sesión"), historyControlsRow_);
    historySessionCombo_ = new QComboBox(historyControlsRow_);
    historySessionCombo_->setObjectName("HistorySessionCombo");
    historySessionCombo_->setEnabled(false);

    historyControlsLayout->addWidget(historySessionLabel);
    historyControlsLayout->addWidget(historySessionCombo_, 1);
    historyControlsLayout->addStretch(1);
    historyControlsRow_->setVisible(false);

    historyStatusLabel_ = new QLabel(problemBody_);
    historyStatusLabel_->setObjectName("HistoryStatusLabel");
    historyStatusLabel_->setVisible(false);
    historyStatusLabel_->setWordWrap(true);

    auto *questionSection = new QFrame(problemBody_);
    questionSection->setObjectName("QuestionSection");
    auto *questionLayout = new QVBoxLayout(questionSection);
    questionLayout->setContentsMargins(12, 12, 12, 12);
    questionLayout->setSpacing(12);

    problemStatement_ = new QTextEdit(questionSection);
    problemStatement_->setObjectName("ProblemStatement");
    problemStatement_->setReadOnly(true);
    problemStatement_->setWordWrapMode(QTextOption::WordWrap);
    problemStatement_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    problemStatement_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    problemStatement_->setFixedHeight(220);
    problemStatement_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    questionLayout->addWidget(problemStatement_);

    answerButtons_ = new QButtonGroup(this);
    answerButtons_->setExclusive(true);

    answerOptions_.clear();
    for (int index = 0; index < 4; ++index) {
        auto *option = new QRadioButton(questionSection);
        option->setObjectName(QStringLiteral("AnswerOption_%1").arg(index));
        option->setVisible(false);
        answerButtons_->addButton(option, index);
        answerOptions_.push_back(option);
        questionLayout->addWidget(option);
    }

    questionLayout->addStretch(1);

    submitButton_ = new QPushButton(tr("Comprobar respuesta"), problemBody_);
    submitButton_->setObjectName("SubmitButton");
    submitButton_->setEnabled(false);

    auto *actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->addWidget(prevProblemButton_);
    actionLayout->addWidget(submitButton_, 1);
    actionLayout->addWidget(nextProblemButton_);

    bodyLayout->addWidget(navigationRow_);
    bodyLayout->addWidget(historyControlsRow_);
    bodyLayout->addWidget(historyStatusLabel_);
    bodyLayout->addWidget(questionSection);
    bodyLayout->addStretch(1);
    bodyLayout->addLayout(actionLayout);

    problemLayout->addLayout(problemHeader);
    problemLayout->addWidget(problemBody_);

    contentSplitter_ = new QSplitter(Qt::Horizontal, page);
    contentSplitter_->setObjectName("ContentSplitter");
    contentSplitter_->setHandleWidth(12);
    contentSplitter_->setChildrenCollapsible(false);
    contentSplitter_->addWidget(chartCard);
    contentSplitter_->addWidget(problemCard_);
    contentSplitter_->setStretchFactor(0, 3);
    contentSplitter_->setStretchFactor(1, 2);

    contentStack_ = new QStackedWidget(page);
    contentStack_->setObjectName("ContentStack");
    contentStack_->addWidget(contentSplitter_);
    statisticsPage_ = createStatisticsPage(contentStack_);
    contentStack_->addWidget(statisticsPage_);
    contentStack_->setCurrentWidget(contentSplitter_);

    layout->addWidget(contentStack_, 1);

    auto *viewToggleGroup = new QButtonGroup(this);
    viewToggleGroup->setExclusive(true);
    viewToggleGroup->addButton(questionsToggleButton_);
    viewToggleGroup->addButton(statsButton_);

    connect(questionsToggleButton_, &QToolButton::clicked, this, [this]() {
        setQuestionPanelMode(QuestionPanelMode::Practice);
    });

    connect(statsButton_, &QToolButton::clicked, this, [this]() {
        setQuestionPanelMode(QuestionPanelMode::History);
    });

    if (statisticsButton_) {
        connect(statisticsButton_, &QToolButton::toggled, this, &MainWindow::showStatisticsView);
    }

    connect(profileAction_, &QAction::triggered, this, &MainWindow::showProfileDialog);
    connect(resultsAction_, &QAction::triggered, this, &MainWindow::showResultsDialog);
    connect(logoutAction_, &QAction::triggered, this, &MainWindow::logout);

    connect(problemCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::loadProblemFromSelection);
    connect(randomButton_, &QPushButton::clicked, this, &MainWindow::loadRandomProblem);
    if (historySessionCombo_) {
        connect(historySessionCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::handleHistorySessionSelectionChanged);
    }
    connect(prevProblemButton_, &QPushButton::clicked, this, &MainWindow::goToPreviousProblem);
    connect(nextProblemButton_, &QPushButton::clicked, this, &MainWindow::goToNextProblem);
    connect(collapseProblemButton_, &QToolButton::toggled, this, &MainWindow::toggleProblemPanel);
    connect(answerButtons_, &QButtonGroup::idClicked, this, [this](int) {
        submitButton_->setEnabled(true);
        for (auto *radio : answerOptions_) {
            radio->setStyleSheet(QString());
        }
    });
    connect(submitButton_, &QPushButton::clicked, this, &MainWindow::submitAnswer);
    if (contentSplitter_) {
        connect(contentSplitter_, &QSplitter::splitterMoved, this, &MainWindow::handleSplitterMoved);
    }

    toggleProblemPanel(false);
    updateProblemNavigationState();
    applyProblemPaneConstraints();

    return page;
}

QWidget *MainWindow::createStatisticsPage(QWidget *parent) {
    auto *page = new QWidget(parent);
    page->setObjectName("StatisticsPage");
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(tr("Panel de estadísticas"), page);
    title->setObjectName("StatisticsTitle");

    statsSummaryCard_ = new QFrame(page);
    statsSummaryCard_->setObjectName("StatsSummaryCard");
    auto *summaryLayout = new QGridLayout(statsSummaryCard_);
    summaryLayout->setContentsMargins(24, 20, 24, 20);
    summaryLayout->setSpacing(18);

    const auto createStatBlock = [&](const QString &labelText, QLabel *&valueLabel) {
        auto *container = new QWidget(statsSummaryCard_);
        container->setObjectName("StatsSummaryBlock");
        auto *containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(4);
        containerLayout->setAlignment(Qt::AlignCenter);

        auto *label = new QLabel(labelText, container);
        label->setObjectName("StatsBlockLabel");
        label->setAlignment(Qt::AlignCenter);

        valueLabel = new QLabel(QStringLiteral("--"), container);
        valueLabel->setObjectName("StatsBlockValue");
        valueLabel->setAlignment(Qt::AlignCenter);

        containerLayout->addWidget(label);
        containerLayout->addWidget(valueLabel);
        return container;
    };

    summaryLayout->addWidget(createStatBlock(tr("Respondidas"), statsTotalValueLabel_), 0, 0);
    summaryLayout->addWidget(createStatBlock(tr("Correctas"), statsCorrectValueLabel_), 0, 1);
    summaryLayout->addWidget(createStatBlock(tr("Incorrectas"), statsIncorrectValueLabel_), 0, 2);
    summaryLayout->addWidget(createStatBlock(tr("Precisión"), statsAccuracyValueLabel_), 0, 3);
    summaryLayout->setColumnStretch(0, 1);
    summaryLayout->setColumnStretch(1, 1);
    summaryLayout->setColumnStretch(2, 1);
    summaryLayout->setColumnStretch(3, 1);

    statsChartCard_ = new QFrame(page);
    statsChartCard_->setObjectName("StatsChartCard");
    auto *chartLayout = new QVBoxLayout(statsChartCard_);
    chartLayout->setContentsMargins(24, 20, 24, 24);
    chartLayout->setSpacing(12);

    auto *chartTitle = new QLabel(tr("Tendencia y distribución"), statsChartCard_);
    chartTitle->setObjectName("StatsBlockLabel");
    chartLayout->addWidget(chartTitle);

    auto *chartContentLayout = new QHBoxLayout();
    chartContentLayout->setContentsMargins(0, 0, 0, 0);
    chartContentLayout->setSpacing(16);

    statsTrendWidget_ = new StatsTrendWidget(statsChartCard_);
    statsPieWidget_ = new StatsPieWidget(statsChartCard_);
    chartContentLayout->addWidget(statsTrendWidget_, 3);
    chartContentLayout->addWidget(statsPieWidget_, 2);

    chartLayout->addLayout(chartContentLayout);

    statsTableCard_ = new QFrame(page);
    statsTableCard_->setObjectName("StatsTableCard");
    auto *tableLayout = new QVBoxLayout(statsTableCard_);
    tableLayout->setContentsMargins(24, 20, 24, 24);
    tableLayout->setSpacing(12);

    auto *tableTitle = new QLabel(tr("Sesiones recientes"), statsTableCard_);
    tableTitle->setObjectName("StatsBlockLabel");

    statsSessionsTable_ = new QTableWidget(statsTableCard_);
    statsSessionsTable_->setObjectName("StatsSessionsTable");
    statsSessionsTable_->setColumnCount(5);
    statsSessionsTable_->setHorizontalHeaderLabels({tr("Fecha"), tr("Respondidas"), tr("Correctas"), tr("Incorrectas"), tr("Precisión")});
    statsSessionsTable_->horizontalHeader()->setStretchLastSection(true);
    statsSessionsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    statsSessionsTable_->verticalHeader()->setVisible(false);
    statsSessionsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statsSessionsTable_->setSelectionMode(QAbstractItemView::NoSelection);
    statsSessionsTable_->setFocusPolicy(Qt::NoFocus);
    statsSessionsTable_->setShowGrid(false);
    statsSessionsTable_->setAlternatingRowColors(true);

    tableLayout->addWidget(tableTitle);
    tableLayout->addWidget(statsSessionsTable_, 1);

    statsEmptyStateLabel_ = new QLabel(tr("Todavía no hay datos de práctica. Responde algunas preguntas para generar estadísticas."), page);
    statsEmptyStateLabel_->setAlignment(Qt::AlignCenter);
    statsEmptyStateLabel_->setWordWrap(true);
    statsEmptyStateLabel_->setVisible(false);

    layout->addWidget(title);
    layout->addWidget(statsSummaryCard_);
    layout->addWidget(statsChartCard_);
    layout->addWidget(statsTableCard_, 1);
    layout->addWidget(statsEmptyStateLabel_, 0, Qt::AlignCenter);

    return page;
}

QWidget *MainWindow::createToolStrip(QWidget *parent) {
    auto *strip = new QFrame(parent);
    strip->setObjectName("ToolStrip");
    auto *layout = new QHBoxLayout(strip);
    layout->setContentsMargins(18, 12, 18, 12);
    layout->setSpacing(10);

    buildToolButtons(strip);
    layout->addStretch(1);
    return strip;
}

void MainWindow::buildToolButtons(QWidget *toolStrip) {
    auto *layout = qobject_cast<QHBoxLayout *>(toolStrip->layout());
    if (!layout) {
        return;
    }

    toolActionGroup_ = new QActionGroup(this);
    toolActionGroup_->setExclusive(true);
    connect(toolActionGroup_, &QActionGroup::triggered, this, &MainWindow::setToolFromAction);

    const auto addSeparator = [&]() {
        auto *separator = new QFrame(toolStrip);
        separator->setObjectName("ToolStripSeparator");
        separator->setFrameShape(QFrame::VLine);
        separator->setFrameShadow(QFrame::Sunken);
        separator->setFixedHeight(32);
        layout->addWidget(separator);
    };

    const auto addToolButton = [&](QAction *action,
                                   const QSize &iconSize = QSize(28, 28),
                                   const QString &role = QStringLiteral("drawing-tool")) {
        auto *button = new QToolButton(toolStrip);
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        button->setCheckable(action->isCheckable());
        button->setIconSize(iconSize);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        if (!role.isEmpty()) {
            button->setProperty("role", role);
        }
        layout->addWidget(button);
        return button;
    };

    const auto addToolAction = [&](QAction *&action, const QString &icon, const QString &text, ChartScene::Tool tool) {
        action = new QAction(QIcon(icon), text, this);
        action->setCheckable(true);
        action->setData(static_cast<int>(tool));
        action->setToolTip(text);
        toolActionGroup_->addAction(action);
        addToolButton(action);
    };

    handAction_ = new QAction(QIcon(":/resources/images/icon_hand.svg"), tr("Mover carta"), this);
    handAction_->setCheckable(true);
    handAction_->setToolTip(tr("Haz clic y arrastra para desplazar la carta"));
    toolActionGroup_->addAction(handAction_);
    addToolButton(handAction_);

    addToolAction(pointAction_, ":/resources/images/icon_point.svg", tr("Punto"), ChartScene::Tool::Point);
    addToolAction(lineAction_, ":/resources/images/icon_line.svg", tr("Línea"), ChartScene::Tool::Line);
    addToolAction(arcAction_, ":/resources/images/icon_arc.svg", tr("Arco"), ChartScene::Tool::Arc);
    addToolAction(textAction_, ":/resources/images/icon_text.svg", tr("Texto"), ChartScene::Tool::Text);

    addSeparator();

    colorButton_ = new QToolButton(toolStrip);
    colorButton_->setObjectName("ColorDropdownButton");
    colorButton_->setToolTip(tr("Seleccionar color de trazo"));
    colorButton_->setIconSize(QSize(22, 22));
    colorButton_->setAutoRaise(false);
    colorButton_->setCheckable(false);
    colorButton_->setPopupMode(QToolButton::InstantPopup);
    colorButton_->setCursor(Qt::PointingHandCursor);
    colorButton_->setFocusPolicy(Qt::NoFocus);
    colorButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    colorButton_->setFixedHeight(colorButton_->iconSize().height() + 12);
    colorButton_->setFixedWidth(colorButton_->iconSize().width() + 26);

    colorMenu_ = new QMenu(colorButton_);
    colorMenu_->setObjectName("ColorDropdownMenu");
    colorMenu_->setSeparatorsCollapsible(true);

    colorActionGroup_ = new QActionGroup(colorMenu_);
    colorActionGroup_->setExclusive(true);
    connect(colorActionGroup_, &QActionGroup::triggered, this, &MainWindow::handleColorActionTriggered);

    colorButton_->setMenu(colorMenu_);
    layout->addWidget(colorButton_);

    addSeparator();

    addToolAction(eraserAction_, ":/resources/images/icon_eraser.svg", tr("Borrador"), ChartScene::Tool::Eraser);

    clearAction_ = new QAction(QIcon(":/resources/images/icon_clean.svg"), tr("Reiniciar carta"), this);
    clearAction_->setToolTip(tr("Eliminar todas las marcas"));
    connect(clearAction_, &QAction::triggered, this, &MainWindow::clearChart);
    addToolButton(clearAction_, QSize(26, 26), QStringLiteral("utility-tool"));

    addSeparator();

    protractorAction_ = new QAction(QIcon(":/resources/images/icon_protractor.svg"), tr("Mostrar transportador"), this);
    protractorAction_->setCheckable(true);
    connect(protractorAction_, &QAction::toggled, this, &MainWindow::toggleProtractor);
    addToolButton(protractorAction_, QSize(26, 26), QStringLiteral("utility-tool"));

    rulerAction_ = new QAction(QIcon(":/resources/images/icon_ruler.svg"), tr("Mostrar regla"), this);
    rulerAction_->setCheckable(true);
    connect(rulerAction_, &QAction::toggled, this, &MainWindow::toggleRuler);
    addToolButton(rulerAction_, QSize(26, 26), QStringLiteral("utility-tool"));

    addSeparator();

    addToolAction(crosshairAction_, ":/resources/images/icon_crosshair.svg", tr("Mira"), ChartScene::Tool::Crosshair);

    addSeparator();

    zoomOutAction_ = new QAction(QIcon(":/resources/images/icon_zoom_out.svg"), tr("Alejar"), this);
    connect(zoomOutAction_, &QAction::triggered, this, &MainWindow::zoomOutOnChart);
    addToolButton(zoomOutAction_, QSize(24, 24), QStringLiteral("utility-tool"));

    zoomInAction_ = new QAction(QIcon(":/resources/images/icon_zoom_in.svg"), tr("Acercar"), this);
    connect(zoomInAction_, &QAction::triggered, this, &MainWindow::zoomInOnChart);
    addToolButton(zoomInAction_, QSize(24, 24), QStringLiteral("utility-tool"));

    addSeparator();

    fullScreenAction_ = new QAction(QIcon(":/resources/images/icon_fullscreen.svg"), tr("Pantalla completa"), this);
    fullScreenAction_->setCheckable(true);
    fullScreenAction_->setToolTip(tr("Mostrar la carta sin distracciones"));
    connect(fullScreenAction_, &QAction::toggled, this, &MainWindow::toggleFullscreenMode);
    addToolButton(fullScreenAction_, QSize(26, 26), QStringLiteral("utility-tool"));

    if (handAction_) {
        handAction_->setChecked(true);
        setToolFromAction(handAction_);
    }

    updateToolStripLayout();
}

void MainWindow::updateToolStripLayout() {
    if (!toolStrip_) {
        return;
    }

    auto *layout = qobject_cast<QHBoxLayout *>(toolStrip_->layout());
    if (!layout) {
        return;
    }

    const bool compact = !problemPanelCollapsed_;
    const QMargins margins = compact ? QMargins(12, 10, 12, 10) : QMargins(18, 12, 18, 12);
    layout->setContentsMargins(margins);
    layout->setSpacing(compact ? 8 : 12);
    layout->setAlignment(compact ? (Qt::AlignLeft | Qt::AlignVCenter) : (Qt::AlignHCenter | Qt::AlignVCenter));

    const QSize iconSize = compact ? QSize(24, 24) : QSize(28, 28);

    const int stripHeight = compact ? 60 : 72;
    toolStrip_->setFixedHeight(stripHeight);

    const auto buttons = toolStrip_->findChildren<QToolButton *>(QString(), Qt::FindDirectChildrenOnly);
    for (QToolButton *button : buttons) {
        if (button == colorButton_) {
            const QSize iconSize = colorButton_->iconSize();
            const int widthPadding = compact ? 18 : 26;
            const int heightPadding = compact ? 8 : 12;
            colorButton_->setFixedWidth(iconSize.width() + widthPadding);
            colorButton_->setFixedHeight(iconSize.height() + heightPadding);
            continue;
        }

        button->setIconSize(iconSize);
    }
}

void MainWindow::populateProblems() {
    if (!problemCombo_) {
        return;
    }

    QSignalBlocker blocker(problemCombo_);
    problemCombo_->clear();

    const auto problems = problemManager_.problems();
    for (const auto &problem : problems) {
        problemCombo_->addItem(QStringLiteral("#%1 · %2").arg(problem.id).arg(problem.category), problem.id);
    }

    problemCombo_->setEnabled(problemCombo_->count() > 0);
    updateProblemNavigationState();
}

void MainWindow::enterApplication(const UserRecord &user, bool guestMode) {
    currentUser_ = user;
    currentSession_ = {};
    currentSession_.timestamp = QDateTime::currentDateTime();
    guestSessionActive_ = guestMode;

    populateProblems();
    if (problemCombo_ && problemCombo_->count() > 0) {
        loadRandomProblem();
    } else {
        problemStatement_->setPlainText(tr("No se han encontrado problemas."));
    }

    updateUserPanel();
    updateSessionLabels();

    if (toolStrip_) {
        toolStrip_->setVisible(true);
    }
    if (questionsToggleButton_) {
        questionsToggleButton_->setEnabled(true);
        QSignalBlocker blocker(questionsToggleButton_);
        questionsToggleButton_->setChecked(true);
    }
    if (statsButton_) {
        statsButton_->setEnabled(true);
    }
    if (statisticsButton_) {
        statisticsButton_->setEnabled(true);
        QSignalBlocker blocker(statisticsButton_);
        statisticsButton_->setChecked(false);
    }
    showStatisticsView(false);
    stack_->setCurrentWidget(appPage_);

    showStatusBanner(tr("Bienvenido/a, %1").arg(user.nickname), 4000);

    setQuestionPanelMode(QuestionPanelMode::Practice);

    if (profileAction_) {
        profileAction_->setEnabled(!guestSessionActive_);
    }
    if (resultsAction_) {
        resultsAction_->setEnabled(!guestSessionActive_);
    }
}

void MainWindow::returnToLogin() {
    if (chartScene_) {
        chartScene_->clearMarks();
        chartScene_->setProtractorVisible(false);
        chartScene_->setRulerVisible(false);
    }

    if (protractorAction_) {
        QSignalBlocker blocker(protractorAction_);
        protractorAction_->setChecked(false);
    }
    if (rulerAction_) {
        QSignalBlocker blocker(rulerAction_);
        rulerAction_->setChecked(false);
    }

    currentUser_.reset();
    currentProblem_.reset();
    currentSession_ = {};
    guestSessionActive_ = false;

    if (toolStrip_) {
        toolStrip_->setVisible(false);
    }
    stack_->setCurrentWidget(loginPage_);

    if (profileAction_) {
        profileAction_->setEnabled(true);
    }
    if (resultsAction_) {
        resultsAction_->setEnabled(true);
    }

    if (pointAction_) {
        QSignalBlocker blocker(pointAction_);
        pointAction_->setChecked(false);
    }
    if (handAction_) {
        QSignalBlocker blocker(handAction_);
        handAction_->setChecked(true);
        setToolFromAction(handAction_);
    }

    if (loginUserEdit_) {
        loginUserEdit_->clear();
        loginUserEdit_->setFocus();
    }
    if (loginPasswordEdit_) {
        loginPasswordEdit_->clear();
    }
    if (loginButton_) {
        loginButton_->setEnabled(false);
    }
    if (loginFeedbackLabel_) {
        loginFeedbackLabel_->clear();
        loginFeedbackLabel_->setVisible(false);
        loginFeedbackLabel_->setStyleSheet(QString());
    }
    resetRegisterForm();

    if (problemCombo_) {
        problemCombo_->clear();
    }
    if (problemStatement_) {
        problemStatement_->clear();
    }
    resetAnswerSelection();
    updateProblemNavigationState();
    if (collapseProblemButton_) {
        QSignalBlocker blocker(collapseProblemButton_);
        collapseProblemButton_->setChecked(false);
    }
    toggleProblemPanel(false);

    historySessionSources_.clear();
    historySessionSelection_ = -1;
    if (historySessionCombo_) {
        historySessionCombo_->clear();
        historySessionCombo_->setEnabled(false);
    }
    if (historyControlsRow_) {
        historyControlsRow_->setVisible(false);
    }

    if (questionsToggleButton_) {
        QSignalBlocker blocker(questionsToggleButton_);
        questionsToggleButton_->setChecked(true);
        questionsToggleButton_->setEnabled(false);
    }
    if (statsButton_) {
        statsButton_->setEnabled(false);
    }
    if (statisticsButton_) {
        QSignalBlocker blocker(statisticsButton_);
        statisticsButton_->setChecked(false);
        statisticsButton_->setEnabled(false);
    }

    showStatisticsView(false);

    userSummaryLabel_->setText(tr("Sin sesión activa"));
    sessionStatsLabel_->setText(tr("Aciertos: 0 · Fallos: 0"));
    userMenuButton_->setIcon(QIcon(makeCircularAvatar(kDefaultAvatarPath)));
    userMenuButton_->setToolTip(QString());

    setQuestionPanelMode(QuestionPanelMode::Practice);
    updateStatisticsPanel();
}

void MainWindow::updateUserPanel() {
    if (!currentUser_) {
        userSummaryLabel_->setText(tr("Sin sesión activa"));
        userMenuButton_->setIcon(QIcon(makeCircularAvatar(kDefaultAvatarPath)));
        return;
    }

    const auto &user = *currentUser_;
    userSummaryLabel_->setText(tr("Hola, %1").arg(user.nickname));

    const QString avatarFile = userManager_.resolvedAvatarPath(user.avatarPath);
    userMenuButton_->setIcon(QIcon(makeCircularAvatar(avatarFile)));
    userMenuButton_->setToolTip(tr("%1\n%2").arg(user.nickname, user.email));
}

void MainWindow::updateSessionLabels() {
    if (!sessionStatsLabel_) {
        return;
    }
    sessionStatsLabel_->setText(tr("Aciertos: %1 · Fallos: %2")
                                    .arg(currentSession_.hits)
                                    .arg(currentSession_.faults));
    updateStatisticsPanel();
}

void MainWindow::updateAnswerOptions() {
    if (!currentProblem_) {
        if (problemStatement_) {
            problemStatement_->clear();
        }
        return;
    }

    const auto &problem = *currentProblem_;
    if (problemStatement_) {
        problemStatement_->setPlainText(problem.text);
    }

    QVector<int> order;
    order.reserve(problem.answers.size());
    for (int i = 0; i < problem.answers.size(); ++i) {
        order.push_back(i);
    }
    std::mt19937 rng(QRandomGenerator::global()->generate());
    std::shuffle(order.begin(), order.end(), rng);

    for (int index = 0; index < answerOptions_.size(); ++index) {
        auto *option = answerOptions_.at(index);
        if (index < order.size()) {
            const auto &answer = problem.answers.at(order.at(index));
            option->setText(answer.text);
            option->setProperty("valid", answer.valid);
            option->setVisible(true);
        } else {
            option->setVisible(false);
        }
        option->setStyleSheet(QString());
        option->setChecked(false);
    }

    submitButton_->setEnabled(false);
}

void MainWindow::recordSessionIfNeeded() {
    if (!currentUser_ || guestSessionActive_) {
        return;
    }

    if (currentSession_.hits == 0 && currentSession_.faults == 0) {
        return;
    }

    SessionRecord session = currentSession_;
    QString error;
    if (!userManager_.appendSession(currentUser_->nickname, session, error)) {
        QMessageBox::warning(this, tr("Guardar sesión"), error);
        return;
    }

    if (auto refreshed = userManager_.getUser(currentUser_->nickname)) {
        currentUser_ = refreshed;
    }

    currentSession_ = {};
    updateSessionLabels();
}

void MainWindow::resetAnswerSelection() {
    if (!answerButtons_) {
        return;
    }
    answerButtons_->setExclusive(false);
    for (auto *button : answerOptions_) {
        button->setChecked(false);
        button->setStyleSheet(QString());
    }
    answerButtons_->setExclusive(true);
    if (submitButton_) {
        submitButton_->setEnabled(false);
    }
}

void MainWindow::applyAppTheme() {
    QFile file(QString::fromLatin1(kLightThemePath));
    QString sheet;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sheet = QString::fromUtf8(file.readAll());
    } else {
        QFile fallback(QString::fromLatin1(kFallbackThemePath));
        if (fallback.open(QIODevice::ReadOnly | QIODevice::Text)) {
            sheet = QString::fromUtf8(fallback.readAll());
        }
    }

    if (!sheet.isEmpty()) {
        qApp->setStyleSheet(sheet);
    }

    refreshColorPalette();
}

void MainWindow::refreshColorPalette() {
    if (!colorButton_) {
        return;
    }

    const QVector<QColor> themePalette{
        QColor("#2f81f7"),
        QColor("#a371f7"),
        QColor("#f0883e"),
        QColor("#3fb950"),
        QColor("#ff5e8a"),
        QColor("#8b949e")};

    QVector<QColor> palette;
    palette.push_back(QColor("#000000"));
    palette += themePalette;
    paletteColors_ = palette;

    if (!colorMenu_) {
        colorMenu_ = new QMenu(colorButton_);
        colorMenu_->setObjectName("ColorDropdownMenu");
        colorButton_->setMenu(colorMenu_);
    }

    if (!colorActionGroup_) {
        colorActionGroup_ = new QActionGroup(colorMenu_);
        colorActionGroup_->setExclusive(true);
        connect(colorActionGroup_, &QActionGroup::triggered, this, &MainWindow::handleColorActionTriggered);
    }

    colorMenu_->clear();
    const auto oldActions = colorActionGroup_->actions();
    for (QAction *action : oldActions) {
        colorActionGroup_->removeAction(action);
        action->deleteLater();
    }

    const QColor borderColor("#d0d7de");
    const QColor backgroundColor("#ffffff");
    const QColor hoverColor("#2f81f7");
    const QString arrowIconPath = QStringLiteral(":/resources/images/icon_chevron_down_light.svg");
    const QString selectionColor = QStringLiteral("rgba(47,129,247,0.15)");

    colorButton_->setStyleSheet(QStringLiteral(
        "QToolButton#ColorDropdownButton { border: 1px solid %1; border-radius: 12px; padding: 2px 14px 2px 6px; "
        "background-color: %2; min-width: 0px; }"
        "QToolButton#ColorDropdownButton:hover { border-color: %3; }"
        "QToolButton#ColorDropdownButton::menu-indicator { image: url(%4); subcontrol-origin: padding; "
        "subcontrol-position: center right; width: 12px; height: 12px; margin-right: 2px; }")
                                  .arg(borderColor.name(), backgroundColor.name(), hoverColor.name(), arrowIconPath));

    const QSize swatchSize(24, 24);
    const QSize buttonSize(swatchSize.width() + 18, swatchSize.height() + 18);

    colorMenu_->setStyleSheet(QStringLiteral(
        "QMenu#ColorDropdownMenu { border: 1px solid %1; border-radius: 12px; padding: 10px; "
        "background-color: %2; }"
        "QMenu#ColorDropdownMenu QWidget { background-color: transparent; }")
                                       .arg(borderColor.name(), backgroundColor.name()));
    colorMenu_->setMinimumWidth(buttonSize.width() + 8);
    QAction *selectedAction = nullptr;
    const QColor currentColor = chartScene_ && chartScene_->currentColor().isValid()
                                    ? chartScene_->currentColor()
                                    : palette.value(0, QColor("#000000"));

    for (const QColor &color : palette) {
        QPixmap swatch(swatchSize.width() + 6, swatchSize.height() + 6);
        swatch.fill(Qt::transparent);
        QPainter painter(&swatch);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QRect rect = swatch.rect().adjusted(3, 3, -3, -3);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect, 8, 8);
        painter.end();

        auto *action = new QWidgetAction(colorMenu_);
        action->setData(color);
        action->setCheckable(true);

        auto *button = new QToolButton(colorMenu_);
        button->setAutoRaise(false);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setIcon(QIcon(swatch));
        button->setIconSize(swatchSize);
        button->setFixedSize(buttonSize);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setFocusPolicy(Qt::NoFocus);
        button->setStyleSheet(QStringLiteral(
            "QToolButton { border: none; border-radius: 14px; padding: 0px; background-color: transparent; }"
            "QToolButton:hover { background-color: rgba(47,129,247,0.12); }"
            "QToolButton:checked { border: 2px solid %1; background-color: %2; }")
                                  .arg(hoverColor.name(), selectionColor));

        action->setDefaultWidget(button);
        colorMenu_->addAction(action);
        colorActionGroup_->addAction(action);

        connect(button, &QToolButton::clicked, this, [this, action]() {
            action->trigger();
            if (colorMenu_) {
                colorMenu_->close();
            }
        });
        connect(action, &QAction::toggled, button, &QToolButton::setChecked);

        if (currentColor == color) {
            selectedAction = action;
        }
    }

    if (!selectedAction) {
        const auto actions = colorActionGroup_->actions();
        if (!actions.isEmpty()) {
            selectedAction = actions.first();
        }
    }

    if (selectedAction) {
        selectedAction->setChecked(true);
        currentColorAction_ = selectedAction;
        handleColorSelection(selectedAction->data().value<QColor>());
    } else {
        updateColorButtonIcon(QColor("#000000"));
    }
}

QPixmap MainWindow::makeCircularAvatar(const QString &avatarPath, int size) const {
    QPixmap source(avatarPath);
    if (source.isNull()) {
        source.load(kDefaultAvatarPath);
    }
    QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap result(size, size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, scaled);
    painter.end();

    return result;
}
