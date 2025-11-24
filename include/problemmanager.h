#pragma once

#include <optional>

#include <QObject>
#include <QString>
#include <QVector>

struct AnswerOption {
    QString text;
    bool valid = false;
};

struct ProblemEntry {
    int id = -1;
    QString category;
    QString text;
    QVector<AnswerOption> answers;
};

class ProblemManager : public QObject {
    Q_OBJECT
public:
    explicit ProblemManager(QString storagePath, QObject *parent = nullptr);

    bool load();
    QVector<ProblemEntry> problems() const;
    std::optional<ProblemEntry> findById(int id) const;
    std::optional<ProblemEntry> randomProblem() const;

signals:
    void problemsChanged();

private:
    QString storagePath_;
    QVector<ProblemEntry> problems_;
};
