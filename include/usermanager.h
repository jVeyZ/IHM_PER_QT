#pragma once

#include <optional>

#include <QDate>
#include <QDateTime>
#include <QImage>
#include <QString>
#include <QVector>

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
    explicit UserManager(QString storagePath, QString avatarsDirectory);

    bool load();
    bool save() const;

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

    QString storagePath_;
    QString avatarsDirectory_;
    QVector<UserRecord> users_;
};
