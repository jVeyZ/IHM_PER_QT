#pragma once

#include <optional>

#include <QDate>
#include <QDateTime>
#include <QImage>
#include <QString>
#include <QVector>

#include "navigation.h"

struct AttemptOption {
    QString text;
    bool correct = false;
};

struct QuestionAttempt {
    QDateTime timestamp;
    int problemId = -1;
    QString question;
    QString selectedAnswer;
    QString correctAnswer;
    bool correct = false;
    QVector<AttemptOption> options;
    int selectedIndex = -1;
};

struct SessionRecord {
    QDateTime timestamp;
    int hits = 0;
    int faults = 0;
    QVector<QuestionAttempt> attempts;
};

struct UserRecord {
    QString nickname;
    QString email;
    QString passwordHash;
    QString salt;
    QDate birthdate;
    QString avatarPath;
    QVector<SessionRecord> sessions;
};

class UserManager {
public:
    explicit UserManager(Navigation &navigation, QString avatarsDirectory);

    bool load();
    bool registerUser(const QString &nickname,
                      const QString &email,
                      const QString &password,
                      const QDate &birthdate,
                      const QString &avatarSource,
                      QString &errorMessage);

    std::optional<UserRecord> authenticate(const QString &nickname,
                                           const QString &password,
                                           QString &errorMessage) const;

    bool updateUser(const QString &nickname,
                    const QString &email,
                    const std::optional<QString> &newPassword,
                    const QDate &birthdate,
                    const QString &avatarSource,
                    QString &errorMessage);
    bool appendSession(const QString &nickname, const SessionRecord &session, QString &errorMessage);

    std::optional<UserRecord> getUser(const QString &nickname) const;
    QVector<UserRecord> allUsers() const;

    QString resolvedAvatarPath(const QString &storedPath) const;

private:
    QString hashPassword(const QString &password, const QString &salt) const;
    QString generateSalt() const;
    QString ensureAvatarStored(const QString &nickname, const QString &sourcePath, QString &errorMessage) const;
    int findIndex(const QString &nickname) const;

    QString encodePasswordPayload(const QString &salt, const QString &hash) const;
    void decodePasswordPayload(const QString &payload, QString &saltOut, QString &hashOut) const;
    UserRecord makeRecordFromNavUser(const User &navUser) const;
    SessionRecord makeRecordFromNavSession(const QString &nickname, const Session &navSession) const;
    QString persistAvatarImage(const QString &nickname, const QImage &image) const;
    QImage loadAvatarImage(const QString &path) const;
    QString resolveDatabasePath() const;
    bool ensureHistoryStorage(QString &errorMessage) const;
    QVector<QuestionAttempt> loadSessionAttempts(const QString &nickname, const QDateTime &sessionTimestamp) const;
    bool storeSessionAttempts(const QString &nickname, const SessionRecord &session, QString &errorMessage) const;
    QString historyConnectionName() const;

    Navigation &navigation_;
    QString avatarsDirectory_;
    QString databasePath_;
    mutable bool historyStorageReady_ = false;
    QVector<UserRecord> users_;
};
