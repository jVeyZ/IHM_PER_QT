#include "usermanager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRandomGenerator>

namespace {
constexpr auto kDefaultAvatarResource = ":/resources/images/default_avatar.svg";
}

UserManager::UserManager(QString storagePath, QString avatarsDirectory)
    : storagePath_(std::move(storagePath)), avatarsDirectory_(std::move(avatarsDirectory)) {
    QFileInfo info(storagePath_);
    if (!info.absolutePath().isEmpty()) {
        QDir().mkpath(info.absolutePath());
    }

    if (!avatarsDirectory_.isEmpty()) {
        QDir().mkpath(avatarsDirectory_);
    }
}

bool UserManager::load() {
    users_.clear();

    QFile file(storagePath_);
    if (!file.exists()) {
        return save();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    const auto usersArray = doc.object().value("users").toArray();
    for (const auto &value : usersArray) {
        if (!value.isObject()) {
            continue;
        }
        const auto obj = value.toObject();
        UserRecord user;
        user.nickname = obj.value("nickname").toString();
        user.email = obj.value("email").toString();
        user.passwordHash = obj.value("passwordHash").toString();
        user.salt = obj.value("salt").toString();
        user.birthdate = QDate::fromString(obj.value("birthdate").toString(), Qt::ISODate);
        user.avatarPath = obj.value("avatarPath").toString(kDefaultAvatarResource);

        const auto sessionsArray = obj.value("sessions").toArray();
        for (const auto &sessionValue : sessionsArray) {
            if (!sessionValue.isObject()) {
                continue;
            }
            const auto sessionObj = sessionValue.toObject();
            SessionRecord session;
            session.timestamp = QDateTime::fromString(sessionObj.value("timestamp").toString(), Qt::ISODateWithMs);
            session.hits = sessionObj.value("hits").toInt();
            session.faults = sessionObj.value("faults").toInt();
            const auto attemptsArray = sessionObj.value("attempts").toArray();
            for (const auto &attemptValue : attemptsArray) {
                if (!attemptValue.isObject()) {
                    continue;
                }
                const auto attemptObj = attemptValue.toObject();
                QuestionAttempt attempt;
                attempt.timestamp = QDateTime::fromString(attemptObj.value("timestamp").toString(), Qt::ISODateWithMs);
                attempt.problemId = attemptObj.value("problemId").toInt(-1);
                attempt.question = attemptObj.value("question").toString();
                attempt.selectedAnswer = attemptObj.value("selectedAnswer").toString();
                attempt.correctAnswer = attemptObj.value("correctAnswer").toString();
                attempt.correct = attemptObj.value("correct").toBool();
                const auto optionsArray = attemptObj.value("options").toArray();
                for (const auto &optionValue : optionsArray) {
                    if (!optionValue.isObject()) {
                        continue;
                    }
                    const auto optionObj = optionValue.toObject();
                    AttemptOption option;
                    option.text = optionObj.value("text").toString();
                    option.correct = optionObj.value("correct").toBool();
                    attempt.options.push_back(option);
                }
                attempt.selectedIndex = attemptObj.value("selectedIndex").toInt(-1);
                if (attempt.options.isEmpty()) {
                    if (!attempt.selectedAnswer.isEmpty()) {
                        AttemptOption option;
                        option.text = attempt.selectedAnswer;
                        option.correct = attempt.selectedAnswer == attempt.correctAnswer && !attempt.correctAnswer.isEmpty();
                        attempt.options.push_back(option);
                    }
                    if (!attempt.correctAnswer.isEmpty() && attempt.selectedAnswer != attempt.correctAnswer) {
                        AttemptOption option;
                        option.text = attempt.correctAnswer;
                        option.correct = true;
                        attempt.options.push_back(option);
                    }
                }
                session.attempts.push_back(std::move(attempt));
            }
            user.sessions.push_back(session);
        }
        users_.push_back(user);
    }

    return true;
}

bool UserManager::save() const {
    QJsonArray usersArray;
    for (const auto &user : users_) {
        QJsonObject userObj{
            {"nickname", user.nickname},
            {"email", user.email},
            {"passwordHash", user.passwordHash},
            {"salt", user.salt},
            {"birthdate", user.birthdate.toString(Qt::ISODate)},
            {"avatarPath", user.avatarPath.isEmpty() ? QString::fromUtf8(kDefaultAvatarResource) : user.avatarPath}
        };

        QJsonArray sessionsArray;
        for (const auto &session : user.sessions) {
            QJsonArray attemptsArray;
            for (const auto &attempt : session.attempts) {
                QJsonArray optionsArray;
                for (const auto &option : attempt.options) {
                    optionsArray.push_back(QJsonObject{
                        {"text", option.text},
                        {"correct", option.correct}
                    });
                }

                attemptsArray.push_back(QJsonObject{
                    {"timestamp", attempt.timestamp.toString(Qt::ISODateWithMs)},
                    {"problemId", attempt.problemId},
                    {"question", attempt.question},
                    {"selectedAnswer", attempt.selectedAnswer},
                    {"correctAnswer", attempt.correctAnswer},
                    {"correct", attempt.correct},
                    {"options", optionsArray},
                    {"selectedIndex", attempt.selectedIndex}
                });
            }

            sessionsArray.push_back(QJsonObject{
                {"timestamp", session.timestamp.toString(Qt::ISODateWithMs)},
                {"hits", session.hits},
                {"faults", session.faults},
                {"attempts", attemptsArray}
            });
        }
        userObj.insert("sessions", sessionsArray);
        usersArray.push_back(userObj);
    }

    QJsonObject root{
        {"users", usersArray}
    };

    QFile file(storagePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool UserManager::registerUser(const QString &nickname,
                               const QString &email,
                               const QString &password,
                               const QDate &birthdate,
                               const QString &avatarSource,
                               QString &errorMessage) {
    if (findIndex(nickname) != -1) {
        errorMessage = QObject::tr("El nombre de usuario ya está en uso.");
        return false;
    }

    UserRecord user;
    user.nickname = nickname;
    user.email = email;
    user.birthdate = birthdate;
    user.salt = generateSalt();
    user.passwordHash = hashPassword(password, user.salt);

    QString avatarError;
    if (!avatarSource.isEmpty()) {
        user.avatarPath = ensureAvatarStored(nickname, avatarSource, avatarError);
        if (user.avatarPath.isEmpty()) {
            errorMessage = avatarError;
            return false;
        }
    } else {
        user.avatarPath = QString::fromUtf8(kDefaultAvatarResource);
    }

    users_.push_back(user);
    if (!save()) {
        users_.pop_back();
        errorMessage = QObject::tr("No se pudo guardar el nuevo usuario en disco.");
        return false;
    }

    return true;
}

std::optional<UserRecord> UserManager::authenticate(const QString &nickname,
                                                    const QString &password,
                                                    QString &errorMessage) const {
    const int index = findIndex(nickname);
    if (index == -1) {
        errorMessage = QObject::tr("Usuario o contraseña incorrectos.");
        return std::nullopt;
    }

    const auto &user = users_.at(index);
    const auto hash = hashPassword(password, user.salt);
    if (!hash.isEmpty() && hash == user.passwordHash) {
        return user;
    }

    errorMessage = QObject::tr("Usuario o contraseña incorrectos.");
    return std::nullopt;
}

bool UserManager::updateUser(const QString &nickname,
                             const QString &email,
                             const std::optional<QString> &newPassword,
                             const QDate &birthdate,
                             const QString &avatarSource,
                             QString &errorMessage) {
    const int index = findIndex(nickname);
    if (index == -1) {
        errorMessage = QObject::tr("El usuario no existe.");
        return false;
    }

    auto &user = users_[index];
    const UserRecord previous = user;
    user.email = email;
    user.birthdate = birthdate;

    if (newPassword.has_value()) {
        user.salt = generateSalt();
        user.passwordHash = hashPassword(newPassword.value(), user.salt);
    }

    if (!avatarSource.isEmpty()) {
        QString avatarError;
        const auto storedPath = ensureAvatarStored(user.nickname, avatarSource, avatarError);
        if (storedPath.isEmpty()) {
            user = previous;
            errorMessage = avatarError;
            return false;
        }
        user.avatarPath = storedPath;
    }

    if (!save()) {
        user = previous;
        errorMessage = QObject::tr("No se pudieron guardar los cambios del perfil.");
        return false;
    }

    return true;
}

bool UserManager::appendSession(const QString &nickname,
                                const SessionRecord &session,
                                QString &errorMessage) {
    const int index = findIndex(nickname);
    if (index == -1) {
        errorMessage = QObject::tr("El usuario no existe.");
        return false;
    }

    users_[index].sessions.push_back(session);
    if (!save()) {
        errorMessage = QObject::tr("No se pudo guardar la sesión.");
        return false;
    }

    return true;
}

std::optional<UserRecord> UserManager::getUser(const QString &nickname) const {
    const int index = findIndex(nickname);
    if (index == -1) {
        return std::nullopt;
    }
    return users_.at(index);
}

QVector<UserRecord> UserManager::allUsers() const {
    return users_;
}

QString UserManager::resolvedAvatarPath(const QString &storedPath) const {
    if (storedPath.startsWith(QLatin1String(":/"))) {
        return storedPath;
    }
    return avatarsDirectory_.isEmpty() ? storedPath : avatarsDirectory_ + QLatin1Char('/') + storedPath;
}

QString UserManager::hashPassword(const QString &password, const QString &salt) const {
    const auto salted = (salt + password).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex());
}

QString UserManager::generateSalt() const {
    return QString::number(QRandomGenerator::global()->generate64(), 16);
}

QString UserManager::ensureAvatarStored(const QString &nickname,
                                        const QString &sourcePath,
                                        QString &errorMessage) const {
    if (sourcePath.startsWith(QLatin1String(":/")) || avatarsDirectory_.isEmpty()) {
        return sourcePath;
    }

    QFile sourceFile(sourcePath);
    if (!sourceFile.exists()) {
        errorMessage = QObject::tr("No se encuentra la imagen seleccionada.");
        return {};
    }

    const QString extension = QFileInfo(sourceFile).suffix();
    const QString targetName = nickname + QLatin1String("_") + QString::number(QDateTime::currentSecsSinceEpoch()) + QLatin1String(".") + extension;
    const QString targetPath = avatarsDirectory_ + QLatin1Char('/') + targetName;

    if (!sourceFile.copy(targetPath)) {
        errorMessage = QObject::tr("No se pudo copiar la imagen seleccionada.");
        return {};
    }

    QFile::setPermissions(targetPath, QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther | QFile::WriteUser);
    return targetName;
}

int UserManager::findIndex(const QString &nickname) const {
    for (int i = 0; i < users_.size(); ++i) {
        if (users_.at(i).nickname.compare(nickname, Qt::CaseInsensitive) == 0) {
            return i;
        }
    }
    return -1;
}
