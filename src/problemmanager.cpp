#include "problemmanager.h"

#include <QRandomGenerator>
#include <utility>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>

ProblemManager::ProblemManager(Navigation &navigation, QObject *parent)
    : QObject(parent), navigation_(navigation) {}

bool ProblemManager::load() {
    problems_.clear();

    // Try to load from DB directly to handle "true"/"false" strings correctly
    QString dbPath;
    QDir appDir(QCoreApplication::applicationDirPath());
    
    QStringList probes{
        QStringLiteral("navdb.sqlite"),
        QStringLiteral("../navdb.sqlite"),
        QStringLiteral("../Resources/navdb.sqlite")
    };
    
    for (const auto &probe : probes) {
        const QString candidate = appDir.absoluteFilePath(probe);
        if (QFileInfo::exists(candidate)) {
            dbPath = candidate;
            break;
        }
    }
    
    if (dbPath.isEmpty()) {
        QDir walker(appDir);
        for (int depth = 0; depth < 5 && walker.cdUp(); ++depth) {
            const QString candidate = walker.absoluteFilePath(QStringLiteral("navdb.sqlite"));
            if (QFileInfo::exists(candidate)) {
                dbPath = candidate;
                break;
            }
        }
    }

    if (!dbPath.isEmpty()) {
        {
            const QString connectionName = QStringLiteral("ProblemManagerConnection");
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
            db.setDatabaseName(dbPath);
            if (db.open()) {
                QSqlQuery query(db);
                if (query.exec(QStringLiteral("SELECT text, answer1, val1, answer2, val2, answer3, val3, answer4, val4 FROM problem"))) {
                    int nextId = 1;
                    const QString defaultCategory = tr("Banco navdb");
                    
                    while (query.next()) {
                        ProblemEntry entry;
                        entry.id = nextId++;
                        entry.category = defaultCategory;
                        entry.text = query.value(0).toString();
                        
                        for (int i = 1; i <= 7; i += 2) {
                            QString ansText = query.value(i).toString();
                            QString valText = query.value(i+1).toString();
                            
                            if (!ansText.isEmpty()) {
                                AnswerOption option;
                                option.text = ansText;
                                // Handle "true"/"false" strings and integers
                                const QString v = valText.trimmed();
                                if (v.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
                                    option.valid = true;
                                } else if (v.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
                                    option.valid = false;
                                } else {
                                    bool ok;
                                    const int intVal = v.toInt(&ok);
                                    if (ok) {
                                        option.valid = (intVal != 0);
                                    } else {
                                        option.valid = false;
                                    }
                                }
                                entry.answers.push_back(option);
                            }
                        }
                        
                        if (!entry.answers.isEmpty()) {
                            problems_.push_back(std::move(entry));
                        }
                    }
                }
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(QStringLiteral("ProblemManagerConnection"));
    }

    if (!problems_.isEmpty()) {
        emit problemsChanged();
        return true;
    }

    // Fallback to Navigation if DB load failed
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
