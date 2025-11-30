#include "problemmanager.h"

#include <QRandomGenerator>
#include <utility>

ProblemManager::ProblemManager(Navigation &navigation, QObject *parent)
    : QObject(parent), navigation_(navigation) {}

bool ProblemManager::load() {
    problems_.clear();
    navigation_.reload();

    const auto &navProblems = navigation_.problems();
    problems_.reserve(navProblems.size());

    int nextId = 1;
    const QString defaultCategory = tr("Banco navdb");

    for (const auto &navProblem : navProblems) {
        ProblemEntry entry;
        entry.id = nextId++;
        entry.category = defaultCategory;
        entry.text = navProblem.text();

        const auto &answers = navProblem.answers();
        entry.answers.reserve(answers.size());
        for (const auto &answer : answers) {
            AnswerOption option;
            option.text = answer.text();
            option.valid = answer.validity();
            entry.answers.push_back(option);
        }

        if (!entry.answers.isEmpty()) {
            problems_.push_back(std::move(entry));
        }
    }

    emit problemsChanged();
    return true;
}

QVector<ProblemEntry> ProblemManager::problems() const {
    return problems_;
}

std::optional<ProblemEntry> ProblemManager::findById(int id) const {
    for (const auto &problem : problems_) {
        if (problem.id == id) {
            return problem;
        }
    }
    return std::nullopt;
}

std::optional<ProblemEntry> ProblemManager::randomProblem() const {
    if (problems_.isEmpty()) {
        return std::nullopt;
    }
    const int index = QRandomGenerator::global()->bounded(problems_.size());
    return problems_.at(index);
}
