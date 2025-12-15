#include "navigationdao.h"

#include <QSqlDatabase>
#include <QVariant>

NavigationDAO::NavigationDAO(const QString &dbFilePath)
    : m_dbFilePath(dbFilePath)
{
    m_connectionName = QStringLiteral("navdb_%1")
        .arg(reinterpret_cast<quintptr>(this));

    open();
    createTablesIfNeeded();
}

NavigationDAO::~NavigationDAO()
{
    close();
}

void NavigationDAO::open()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        m_db = QSqlDatabase::database(m_connectionName);
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
        m_db.setDatabaseName(m_dbFilePath);
    }

    if (!m_db.open()) {
        throw NavDAOException(
            QStringLiteral("NavigationDAO: error opening database '%1': %2")
                .arg(m_dbFilePath, m_db.lastError().text()));
    }

    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
}

void NavigationDAO::close()
{
    if (m_db.isOpen())
        m_db.close();

    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

void NavigationDAO::createTablesIfNeeded()
{
    createUserTable();
    createSessionTable();
    createProblemTable();
}

void NavigationDAO::createUserTable()
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS user ("
        "nickName   TEXT,"
        "email      TEXT NOT NULL,"
        "password   TEXT NOT NULL,"
        "avatar     BLOB,"
        "birthdate  TEXT NOT NULL,"
        "PRIMARY KEY(nickName)"
        ") WITHOUT ROWID;";

    QSqlQuery q(m_db);
    if (!q.exec(QString::fromUtf8(sql))) {
        throwSqlError("createUserTable", q.lastError());
    }
}

void NavigationDAO::createSessionTable()
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS session ("
        "userNickName TEXT,"
        "timeStamp    TEXT,"
        "hits         INTEGER,"
        "faults       INTEGER,"
        "FOREIGN KEY(userNickName)"
        "  REFERENCES user(nickName)"
        "  ON UPDATE CASCADE"
        "  ON DELETE CASCADE"
        ");";

    QSqlQuery q(m_db);
    if (!q.exec(QString::fromUtf8(sql))) {
        throwSqlError("createSessionTable", q.lastError());
    }
}

void NavigationDAO::createProblemTable()
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS problem ("
        "text    TEXT,"
        "answer1 TEXT,"
        "val1    BOOLEAN,"
        "answer2 TEXT,"
        "val2    BOOLEAN,"
        "answer3 TEXT,"
        "val3    BOOLEAN,"
        "answer4 TEXT,"
        "val4    BOOLEAN"
        ");";

    QSqlQuery q(m_db);
    if (!q.exec(QString::fromUtf8(sql))) {
        throwSqlError("createProblemTable", q.lastError());
    }
}

QMap<QString, User> NavigationDAO::loadUsers()
{
    QMap<QString, User> result;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT nickName, email, password, avatar, birthdate FROM user;"))) {
        throwSqlError("loadUsers", q.lastError());
    }

    while (q.next()) {
        User u = buildUserFromQuery(q);
        QVector<Session> sessions = loadSessionsFor(u.nickName());
        u.setSessions(sessions);
        result.insert(u.nickName(), u);
    }

    return result;
}

QVector<Problem> NavigationDAO::loadProblems()
{
    QVector<Problem> result;

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT * FROM problem;"))) {
        throwSqlError("loadProblems", q.lastError());
    }

    while (q.next()) {
        result.push_back(buildProblemFromQuery(q));
    }
    return result;
}

void NavigationDAO::saveUser(User &user)
{
    if (user.insertedInDb()) {
        updateUser(user);
        return;
    }

    const char *sql =
        "INSERT INTO user(nickName, password, email, birthdate, avatar) "
        "VALUES(?,?,?,?,?);";

    QSqlQuery q(m_db);
    if (!q.prepare(QString::fromUtf8(sql))) {
        throwSqlError("saveUser.prepare", q.lastError());
    }

    q.bindValue(0, user.nickName());
    q.bindValue(1, user.password());
    q.bindValue(2, user.email());
    q.bindValue(3, dateToDb(user.birthdate()));
    q.bindValue(4, imageToPng(user.avatar()));

    if (!q.exec()) {
        throwSqlError("saveUser.exec", q.lastError());
    }

    user.setInsertedInDb(true);

    for (const Session &s : user.sessions()) {
        addSession(user.nickName(), s);
    }
}

void NavigationDAO::updateUser(const User &user)
{
    const char *sql =
        "UPDATE user SET email=?, password=?, avatar=?, birthdate=? "
        "WHERE nickName=?;";

    QSqlQuery q(m_db);
    if (!q.prepare(QString::fromUtf8(sql))) {
        throwSqlError("updateUser.prepare", q.lastError());
    }

    q.bindValue(0, user.email());
    q.bindValue(1, user.password());
    q.bindValue(2, imageToPng(user.avatar()));
    q.bindValue(3, dateToDb(user.birthdate()));
    q.bindValue(4, user.nickName());

    if (!q.exec()) {
        throwSqlError("updateUser.exec", q.lastError());
    }
}

void NavigationDAO::deleteUser(const QString &nickName)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM user WHERE nickName=?;"));
    q.bindValue(0, nickName);

    if (!q.exec()) {
        throwSqlError("deleteUser", q.lastError());
    }
}

QVector<Session> NavigationDAO::loadSessionsFor(const QString &nickName)
{
    QVector<Session> res;

    const char *sql =
        "SELECT timeStamp, hits, faults FROM session "
        "WHERE userNickName=?;";

    QSqlQuery q(m_db);
    if (!q.prepare(QString::fromUtf8(sql))) {
        throwSqlError("loadSessionsFor.prepare", q.lastError());
    }
    q.bindValue(0, nickName);

    if (!q.exec()) {
        throwSqlError("loadSessionsFor.exec", q.lastError());
    }

    while (q.next()) {
        res.push_back(buildSessionFromQuery(q));
    }
    return res;
}

void NavigationDAO::addSession(const QString &nickName, const Session &session)
{
    const char *sql =
        "INSERT INTO session(userNickName, timeStamp, hits, faults) "
        "VALUES(?,?,?,?);";

    QSqlQuery q(m_db);
    if (!q.prepare(QString::fromUtf8(sql))) {
        throwSqlError("addSession.prepare", q.lastError());
    }

    q.bindValue(0, nickName);
    q.bindValue(1, dateTimeToDb(session.timeStamp()));
    q.bindValue(2, session.hits());
    q.bindValue(3, session.faults());

    if (!q.exec()) {
        throwSqlError("addSession.exec", q.lastError());
    }
}

void NavigationDAO::replaceAllProblems(const QVector<Problem> &problems)
{
    {
        QSqlQuery del(m_db);
        if (!del.exec(QStringLiteral("DELETE FROM problem;"))) {
            throwSqlError("replaceAllProblems.DELETE", del.lastError());
        }
    }

    const char *sql =
        "INSERT INTO problem(text, answer1, val1, answer2, val2, "
        "answer3, val3, answer4, val4) "
        "VALUES(?,?,?,?,?,?,?,?,?);";

    QSqlQuery q(m_db);
    if (!q.prepare(QString::fromUtf8(sql))) {
        throwSqlError("replaceAllProblems.prepare", q.lastError());
    }

    for (const Problem &p : problems) {
        QVector<Answer> ans = p.answers();
        while (ans.size() < 4) {
            ans.push_back(Answer(QString(), false));
        }

        q.bindValue(0, p.text());
        q.bindValue(1, ans[0].text());
        q.bindValue(2, boolToDb(ans[0].validity()));
        q.bindValue(3, ans[1].text());
        q.bindValue(4, boolToDb(ans[1].validity()));
        q.bindValue(5, ans[2].text());
        q.bindValue(6, boolToDb(ans[2].validity()));
        q.bindValue(7, ans[3].text());
        q.bindValue(8, boolToDb(ans[3].validity()));

        if (!q.exec()) {
            throwSqlError("replaceAllProblems.exec", q.lastError());
        }
        q.finish();
    }
}

User NavigationDAO::buildUserFromQuery(QSqlQuery &q)
{
    const QString nick  = q.value(QStringLiteral("nickName")).toString();
    const QString email = q.value(QStringLiteral("email")).toString();
    const QString pass  = q.value(QStringLiteral("password")).toString();
    const QByteArray avatarBytes = q.value(QStringLiteral("avatar")).toByteArray();
    const QString birthStr = q.value(QStringLiteral("birthdate")).toString();

    QImage avatar = imageFromPng(avatarBytes);
    QDate  birth  = dateFromDb(birthStr);

    User u(nick, email, pass, avatar, birth);
    u.setInsertedInDb(true);
    return u;
}

Session NavigationDAO::buildSessionFromQuery(QSqlQuery &q)
{
    const QString tsStr = q.value(QStringLiteral("timeStamp")).toString();
    const int hits      = q.value(QStringLiteral("hits")).toInt();
    const int faults    = q.value(QStringLiteral("faults")).toInt();

    QDateTime ts = dateTimeFromDb(tsStr);
    return Session(ts, hits, faults);
}

Problem NavigationDAO::buildProblemFromQuery(QSqlQuery &q)
{
    const QString text = q.value(QStringLiteral("text")).toString();

    QVector<Answer> ans;
    ans.reserve(4);
    for (int i = 1; i <= 4; ++i) {
        QString a = q.value(QString("answer%1").arg(i)).toString();
        QString v = q.value(QString("val%1").arg(i)).toString();
        ans.push_back(Answer(a, boolFromDb(v)));
    }

    return Problem(text, ans);
}

QByteArray NavigationDAO::imageToPng(const QImage &img)
{
    if (img.isNull())
        return {};

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    return bytes;
}

QImage NavigationDAO::imageFromPng(const QByteArray &bytes)
{
    QImage img;
    img.loadFromData(bytes, "PNG");
    return img;
}

QString NavigationDAO::dateToDb(const QDate &date) const
{
    return date.toString(Qt::ISODate);
}

QDate NavigationDAO::dateFromDb(const QString &s) const
{
    return QDate::fromString(s, Qt::ISODate);
}

QString NavigationDAO::dateTimeToDb(const QDateTime &dt) const
{
    return dt.toString(Qt::ISODate);
}

QDateTime NavigationDAO::dateTimeFromDb(const QString &s) const
{
    return QDateTime::fromString(s, Qt::ISODate);
}

QString NavigationDAO::boolToDb(bool v) const
{
    return v ? QStringLiteral("1") : QStringLiteral("0");
}

bool NavigationDAO::boolFromDb(const QString &s) const
{
    return (s == QLatin1String("1"));
}

[[noreturn]] void NavigationDAO::throwSqlError(const QString &where, const QSqlError &err) const
{
    throw NavDAOException(QStringLiteral("NavigationDAO [%1]: %2")
                          .arg(where, err.text()));
}
