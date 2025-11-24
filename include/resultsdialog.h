#pragma once

#include <cstdio>

#include <QDialog>
#include <QVector>

#include "usermanager.h"

class QTableWidget;
class QDateEdit;
class QLabel;

class ResultsDialog : public QDialog {
    Q_OBJECT
public:
    ResultsDialog(const QVector<SessionRecord> &sessions, QWidget *parent = nullptr);

private slots:
    void updateFilter();

private:
    void setupUi();
    void populateTable();
    void updateAttemptDetails(int visibleRow);

    QVector<SessionRecord> sessions_;
    QTableWidget *table_ = nullptr;
    QTableWidget *attemptsTable_ = nullptr;
    QDateEdit *fromDateEdit_ = nullptr;
    QLabel *summaryLabel_ = nullptr;
    QLabel *attemptsHeaderLabel_ = nullptr;
    QVector<int> visibleSessionIndexes_;
};
