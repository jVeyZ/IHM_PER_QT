#include "problemmanager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>

ProblemManager::ProblemManager(QString storagePath, QObject *parent)
    : QObject(parent), storagePath_(std::move(storagePath)) {}

bool ProblemManager::load() {
    problems_.clear();

    QFile file(storagePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    const auto root = doc.object();
    const auto array = root.value("problems").toArray();
    problems_.reserve(array.size());

    for (const auto &value : array) {
        if (!value.isObject()) {
            continue;
        }
        const auto obj = value.toObject();
        ProblemEntry problem;
        problem.id = obj.value("id").toInt(-1);
        problem.category = obj.value("category").toString();
        problem.text = obj.value("text").toString();

        const auto answersArray = obj.value("answers").toArray();
        for (const auto &answerValue : answersArray) {
            if (!answerValue.isObject()) {
                continue;
            }
            const auto answerObj = answerValue.toObject();
            AnswerOption answer;
            answer.text = answerObj.value("text").toString();
            answer.valid = answerObj.value("valid").toBool(false);
            problem.answers.push_back(answer);
        }

        if (!problem.answers.isEmpty()) {
            problems_.push_back(problem);
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
