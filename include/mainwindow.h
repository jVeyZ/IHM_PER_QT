#pragma once

#include <optional>

#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QColor>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QRadioButton>
#include <QString>
#include <QTimer>
#include <QVector>

#include "chartscene.h"
#include "chartview.h"
#include "problemmanager.h"
#include "usermanager.h"

class QComboBox;
class QTextEdit;
class QPushButton;
class QToolButton;
class QStackedWidget;
class QLineEdit;
class QMenu;
class QFrame;
class QDateEdit;
class QSplitter;
class QTableWidget;
class StatsTrendWidget;
class StatsPieWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(UserManager &userManager,
               ProblemManager &problemManager,
               QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void setToolFromAction(QAction *action);
    void handleTextRequested(const QPointF &scenePos);
    void handleDistanceMeasured(double pixels, double nauticalMiles);
    void updateStatusMessage(const QString &message);
    void handleColorActionTriggered(QAction *action);
    void loadProblemFromSelection(int index);
    void loadRandomProblem();
    void submitAnswer();
    void showProfileDialog();
    void showResultsDialog();
    void logout();
    void toggleProtractor(bool checked);
    void toggleRuler(bool checked);
    void toggleExtremes();
    void clearChart();
    void zoomInOnChart();
    void zoomOutOnChart();
    void resetChartZoom();
    void attemptLogin();
    void validateLoginForm();
    void showRegistrationForm();
    void showLoginForm();
    void selectRegisterAvatar();
    void validateRegisterForm();
    void handleRegisterSubmit();
    void startGuestSession();
    void goToPreviousProblem();
    void goToNextProblem();
    void toggleProblemPanel(bool collapsed);
    void toggleFullscreenMode(bool checked);

private:
    enum class QuestionPanelMode {
        Practice,
        History
    };

    struct HistorySessionSource;

    void setupUi();
    QWidget *createLoginPage();
    QWidget *createRegisterPage();
    QWidget *createAppPage();
    QWidget *createToolStrip(QWidget *parent);
    QWidget *createStatisticsPage(QWidget *parent);
    void buildToolButtons(QWidget *toolStrip);
    void updateToolStripLayout();
    void populateProblems();
    void enterApplication(const UserRecord &user, bool guestMode = false);
    void returnToLogin();
    QWidget *buildLoginFormPage(QWidget *parent);
    QWidget *buildRegisterFormPage(QWidget *parent);
    void updateUserPanel();
    void updateSessionLabels();
    void updateAnswerOptions();
    void recordSessionIfNeeded();
    void resetAnswerSelection();
    void applyAppTheme();
    void refreshColorPalette();
    void handleColorSelection(const QColor &color);
    void updateColorButtonIcon(const QColor &color);
    QPixmap makeCircularAvatar(const QString &avatarPath, int size = 40) const;
    void resetRegisterForm();
    bool validateRegisterInputs(QString &errorMessage) const;
    void updateProblemNavigationState();
    void handleSplitterMoved(int pos, int index);
    void applyProblemPaneConstraints(bool rememberWidth = true);
    int clampProblemPaneWidth(int totalWidth, int requestedWidth) const;
    void ensureProblemPaneVisible();
    void setQuestionPanelMode(QuestionPanelMode mode);
    void showStatisticsView(bool active);
    void showStatusBanner(const QString &message, int timeoutMs = 0);
    void updateStatisticsPanel();
    void buildHistoryAttempts();
    void updateHistoryDisplay();
    void updateHistoryStatusLabel(const QString &statusText = QString());
    void updateHistoryNavigationState();
    void refreshHistorySessionOptions();
    void handleHistorySessionSelectionChanged(int index);
    const HistorySessionSource *selectedHistorySessionSource() const;

    UserManager &userManager_;
    ProblemManager &problemManager_;

    std::optional<UserRecord> currentUser_;
    SessionRecord currentSession_{};

    QStackedWidget *stack_ = nullptr;
    QWidget *loginPage_ = nullptr;
    QWidget *registerPage_ = nullptr;
    QWidget *appPage_ = nullptr;
    QWidget *loginFormPage_ = nullptr;
    QWidget *registerFormPage_ = nullptr;
    QLineEdit *loginUserEdit_ = nullptr;
    QLineEdit *loginPasswordEdit_ = nullptr;
    QPushButton *loginButton_ = nullptr;
    QPushButton *guestLoginButton_ = nullptr;
    QLabel *loginFeedbackLabel_ = nullptr;
    bool guestSessionActive_ = false;

    QLineEdit *registerNicknameEdit_ = nullptr;
    QLineEdit *registerEmailEdit_ = nullptr;
    QLineEdit *registerPasswordEdit_ = nullptr;
    QLineEdit *registerConfirmPasswordEdit_ = nullptr;
    QDateEdit *registerBirthdateEdit_ = nullptr;
    QLabel *registerAvatarPreview_ = nullptr;
    QLabel *registerFeedbackLabel_ = nullptr;
    QPushButton *registerSubmitButton_ = nullptr;
    QString registerAvatarPath_;

    QToolButton *userMenuButton_ = nullptr;
    QToolButton *questionsToggleButton_ = nullptr;
    QToolButton *statsButton_ = nullptr;
    QToolButton *statisticsButton_ = nullptr;
    QMenu *userMenu_ = nullptr;
    QAction *profileAction_ = nullptr;
    QAction *resultsAction_ = nullptr;
    QAction *logoutAction_ = nullptr;
    QAction *handAction_ = nullptr;

    QWidget *toolStrip_ = nullptr;
    QActionGroup *toolActionGroup_ = nullptr;
    QAction *crosshairAction_ = nullptr;
    QAction *pointAction_ = nullptr;
    QAction *lineAction_ = nullptr;
    QAction *arcAction_ = nullptr;
    QAction *textAction_ = nullptr;
    QAction *fullScreenAction_ = nullptr;
    QAction *eraserAction_ = nullptr;
    QAction *protractorAction_ = nullptr;
    QAction *rulerAction_ = nullptr;
    QAction *clearAction_ = nullptr;
    QAction *zoomInAction_ = nullptr;
    QAction *zoomOutAction_ = nullptr;
    QAction *lastPrimaryToolAction_ = nullptr;

    QVector<QColor> paletteColors_;
    QToolButton *colorButton_ = nullptr;
    QMenu *colorMenu_ = nullptr;
    QActionGroup *colorActionGroup_ = nullptr;
    QAction *currentColorAction_ = nullptr;

    ChartScene *chartScene_ = nullptr;
    ChartView *chartView_ = nullptr;
    QStackedWidget *contentStack_ = nullptr;
    QSplitter *contentSplitter_ = nullptr;
    QFrame *topBar_ = nullptr;
    QWidget *statisticsPage_ = nullptr;

    QComboBox *problemCombo_ = nullptr;
    QTextEdit *problemStatement_ = nullptr;
    QButtonGroup *answerButtons_ = nullptr;
    QVector<QRadioButton *> answerOptions_;
    QPushButton *submitButton_ = nullptr;
    QToolButton *collapseProblemButton_ = nullptr;
    QWidget *problemBody_ = nullptr;
    QPushButton *prevProblemButton_ = nullptr;
    QPushButton *nextProblemButton_ = nullptr;
    QLabel *sessionStatsLabel_ = nullptr;
    QLabel *statusMessageLabel_ = nullptr;
    QLabel *userSummaryLabel_ = nullptr;
    QFrame *problemCard_ = nullptr;
    QWidget *navigationRow_ = nullptr;
    QPushButton *randomButton_ = nullptr;
    QWidget *historyControlsRow_ = nullptr;
    QComboBox *historySessionCombo_ = nullptr;
    QLabel *historyStatusLabel_ = nullptr;
    QFrame *statsSummaryCard_ = nullptr;
    QFrame *statsChartCard_ = nullptr;
    QFrame *statsTableCard_ = nullptr;
    QLabel *statsTotalValueLabel_ = nullptr;
    QLabel *statsCorrectValueLabel_ = nullptr;
    QLabel *statsIncorrectValueLabel_ = nullptr;
    QLabel *statsAccuracyValueLabel_ = nullptr;
    QLabel *statsEmptyStateLabel_ = nullptr;
    StatsTrendWidget *statsTrendWidget_ = nullptr;
    StatsPieWidget *statsPieWidget_ = nullptr;
    QTableWidget *statsSessionsTable_ = nullptr;

    std::optional<ProblemEntry> currentProblem_;
    bool problemPanelCollapsed_ = false;
    QuestionPanelMode panelMode_ = QuestionPanelMode::Practice;
    bool fullScreenModeActive_ = false;
    bool questionPanelVisibleBeforeFullscreen_ = false;
    QuestionPanelMode questionPanelModeBeforeFullscreen_ = QuestionPanelMode::Practice;
    bool topBarVisibleBeforeFullscreen_ = true;
    QVector<QuestionAttempt> historyAttempts_;
    int currentHistoryIndex_ = -1;
    QString submitButtonDefaultText_;
    int lastProblemPaneWidth_ = -1;
    QTimer statusMessageTimer_;
    bool crosshairActive_ = false;
    bool statisticsViewActive_ = false;
    struct HistorySessionSource {
        QString label;
        const QVector<QuestionAttempt> *attempts = nullptr;
        QDateTime timestamp;
        bool isCurrentSession = false;
    };
    QVector<HistorySessionSource> historySessionSources_;
    int historySessionSelection_ = -1;
};
