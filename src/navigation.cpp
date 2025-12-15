#include "navigation.h"
#include "navdaoexception.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace {
QString resolveDatabasePath()
{
    QDir appDir(QCoreApplication::applicationDirPath());
    const QString localPath = appDir.filePath(QStringLiteral("navdb.sqlite"));
    if (QFileInfo::exists(localPath)) {
        return localPath;
    }

    QDir dir(appDir);
    for (int i = 0; i < 4; ++i) {
        if (!dir.cdUp()) {
            break;
        }
        const QString candidate = dir.filePath(QStringLiteral("navdb.sqlite"));
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return localPath;
}
}

Navigation &Navigation::instance()
{
    static Navigation s_instance;
    return s_instance;
}

Navigation::Navigation()
    : m_dao(resolveDatabasePath())
{
    loadFromDb();
}

void Navigation::loadFromDb()
{
    m_users    = m_dao.loadUsers();
    m_problems = m_dao.loadProblems();
}

User *Navigation::findUser(const QString &nick)
{
    auto it = m_users.find(nick);
    if (it == m_users.end())
        return nullptr;
    return &it.value();
}

const User *Navigation::findUser(const QString &nick) const
{
    auto it = m_users.constFind(nick);
    if (it == m_users.constEnd())
        return nullptr;
    return &(*it);
}

User *Navigation::authenticate(const QString &nick, const QString &password)
{
    User *u = findUser(nick);
    if (!u)
        return nullptr;
    return (u->password() == password) ? u : nullptr;
}

void Navigation::addUser(User &user)
{
    const QString &nick = user.nickName();

    if (m_users.contains(nick)) {
        throw NavDAOException(
            QStringLiteral("Navigation::addUser: user '%1' already exists").arg(nick));
    }

    m_dao.saveUser(user);
    m_users.insert(nick, user);
}

void Navigation::updateUser(const User &user)
{
    const QString &nick = user.nickName();

    if (!m_users.contains(nick)) {
        throw NavDAOException(
            QStringLiteral("Navigation::updateUser: user '%1' does not exist").arg(nick));
    }

    m_dao.updateUser(user);
    m_users[nick] = user;
}

void Navigation::removeUser(const QString &nickName)
{
    if (!m_users.contains(nickName)) {
        throw NavDAOException(
            QStringLiteral("Navigation::removeUser: user '%1' does not exist").arg(nickName));
    }

    m_dao.deleteUser(nickName);
    m_users.remove(nickName);
}

void Navigation::addSession(const QString &nickName, const Session &session)
{
    auto it = m_users.find(nickName);
    if (it == m_users.end()) {
        throw NavDAOException(
            QStringLiteral("Navigation::addSession: user '%1' does not exist").arg(nickName));
    }

    m_dao.addSession(nickName, session);
    it.value().addSession(session);
}

void Navigation::reload()
{
    loadFromDb();
}
