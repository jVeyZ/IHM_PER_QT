#include "resultsdialog.h"

#include "usermanager.h"

#include <QAbstractItemView>
#include <QColor>
#include <QDate>
#include <QDateEdit>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

ResultsDialog::ResultsDialog(const QVector<SessionRecord> &sessions, QWidget *parent)
    : QDialog(parent), sessions_(sessions) {
    setWindowTitle(tr("Resultados de sesiones"));
    setModal(true);
    setupUi();
    populateTable();
}

void ResultsDialog::updateFilter() {
    populateTable();
}

void ResultsDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *filterLayout = new QHBoxLayout();
    auto *fromLabel = new QLabel(tr("Mostrar desde:"));
    fromDateEdit_ = new QDateEdit(QDate::currentDate().addMonths(-1));
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    filterLayout->addWidget(fromLabel);
    filterLayout->addWidget(fromDateEdit_);
    filterLayout->addStretch(1);

    layout->addLayout(filterLayout);

    table_ = new QTableWidget(0, 3, this);
    table_->setHorizontalHeaderLabels({tr("Fecha"), tr("Aciertos"), tr("Fallos")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setAlternatingRowColors(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(table_);

    attemptsHeaderLabel_ = new QLabel(tr("Intentos de la sesión"));
    attemptsHeaderLabel_->setStyleSheet(QStringLiteral("font-weight: 600;"));
    layout->addWidget(attemptsHeaderLabel_);

    attemptsTable_ = new QTableWidget(0, 4, this);
    attemptsTable_->setHorizontalHeaderLabels({tr("Pregunta"), tr("Tu respuesta"), tr("Respuesta correcta"), tr("Resultado")});
    attemptsTable_->horizontalHeader()->setStretchLastSection(true);
    attemptsTable_->verticalHeader()->setVisible(false);
    attemptsTable_->setWordWrap(true);
    attemptsTable_->setSelectionMode(QAbstractItemView::NoSelection);
    attemptsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    attemptsTable_->setAlternatingRowColors(true);
    layout->addWidget(attemptsTable_);

    summaryLabel_ = new QLabel();
    summaryLabel_->setAlignment(Qt::AlignRight);
    layout->addWidget(summaryLabel_);

    connect(fromDateEdit_, &QDateEdit::dateChanged, this, [this](const QDate &) { populateTable(); });
    connect(table_, &QTableWidget::currentCellChanged, this, [this](int currentRow, int, int, int) {
        updateAttemptDetails(currentRow);
    });
}

void ResultsDialog::populateTable() {
    const QDate fromDate = fromDateEdit_->date();

    visibleSessionIndexes_.clear();
    table_->setRowCount(0);
    int totalHits = 0;
    int totalFaults = 0;

    for (int index = 0; index < sessions_.size(); ++index) {
        const auto &session = sessions_.at(index);
        if (session.timestamp.date() < fromDate) {
            continue;
        }
        const int row = table_->rowCount();
        table_->insertRow(row);
        visibleSessionIndexes_.push_back(index);

        table_->setItem(row, 0, new QTableWidgetItem(session.timestamp.toString("dd/MM/yyyy hh:mm")));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(session.hits)));
        table_->setItem(row, 2, new QTableWidgetItem(QString::number(session.faults)));

        totalHits += session.hits;
        totalFaults += session.faults;
    }

    summaryLabel_->setText(tr("Total aciertos: %1 | Total fallos: %2")
                               .arg(totalHits)
                               .arg(totalFaults));

    if (!visibleSessionIndexes_.isEmpty()) {
        table_->setCurrentCell(0, 0);
    } else {
        updateAttemptDetails(-1);
    }
}

void ResultsDialog::updateAttemptDetails(int visibleRow) {
    if (!attemptsTable_) {
        return;
    }

    attemptsTable_->clearSpans();
    attemptsTable_->setRowCount(0);

    if (!attemptsHeaderLabel_) {
        return;
    }

    if (visibleRow < 0 || visibleRow >= visibleSessionIndexes_.size()) {
        attemptsHeaderLabel_->setText(tr("Intentos de la sesión"));
        return;
    }

    const auto &session = sessions_.at(visibleSessionIndexes_.at(visibleRow));
    attemptsHeaderLabel_->setText(tr("Intentos de la sesión • %1")
                                      .arg(session.timestamp.toString("dd/MM/yyyy hh:mm")));

    if (session.attempts.isEmpty()) {
        attemptsTable_->setRowCount(1);
        auto *placeholder = new QTableWidgetItem(tr("No se registraron preguntas en esta sesión."));
        placeholder->setFlags(Qt::ItemIsEnabled);
        attemptsTable_->setItem(0, 0, placeholder);
        attemptsTable_->setSpan(0, 0, 1, attemptsTable_->columnCount());
        return;
    }

    const QColor successBg(QStringLiteral("#e6f4ea"));
    const QColor errorBg(QStringLiteral("#fdecea"));
    const QColor highlightBg(QStringLiteral("#e8f1ff"));

    for (const auto &attempt : session.attempts) {
        const int row = attemptsTable_->rowCount();
        attemptsTable_->insertRow(row);

        auto *questionItem = new QTableWidgetItem(attempt.question);
        questionItem->setFlags(Qt::ItemIsEnabled);
        questionItem->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);

        auto *userItem = new QTableWidgetItem(attempt.selectedAnswer);
        userItem->setFlags(Qt::ItemIsEnabled);
        userItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        userItem->setBackground(attempt.correct ? successBg : errorBg);

        auto *correctItem = new QTableWidgetItem(attempt.correctAnswer);
        correctItem->setFlags(Qt::ItemIsEnabled);
        correctItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        correctItem->setBackground(highlightBg);

        auto *resultItem = new QTableWidgetItem(attempt.correct ? tr("Correcta") : tr("Incorrecta"));
        resultItem->setFlags(Qt::ItemIsEnabled);
        resultItem->setTextAlignment(Qt::AlignCenter);
        resultItem->setForeground(attempt.correct ? QColor(QStringLiteral("#1f7a4d"))
                                                 : QColor(QStringLiteral("#b00020")));

        attemptsTable_->setItem(row, 0, questionItem);
        attemptsTable_->setItem(row, 1, userItem);
        attemptsTable_->setItem(row, 2, correctItem);
        attemptsTable_->setItem(row, 3, resultItem);
    }
}
