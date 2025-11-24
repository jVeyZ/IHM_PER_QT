#include "mainwindow.h"
#include "problemmanager.h"
#include "usermanager.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QMessageBox>
#include <QStringList>

#include <cstdlib>

QString dataPath(const QString &relative) {
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList searchRoots;
    QDir walker(appDir);
    searchRoots << walker.absolutePath();
    for (int depth = 0; depth < 5 && walker.cdUp(); ++depth) {
        searchRoots << walker.absolutePath();
    }
    searchRoots.removeDuplicates();

    for (const auto &rootPath : searchRoots) {
        QDir dir(rootPath);
        const QString candidate = QDir::cleanPath(dir.absoluteFilePath(relative));
        QFileInfo info(candidate);
        if (info.exists()) {
            return candidate;
        }
    }

    QDir fallback(appDir);
    return QDir::cleanPath(fallback.absoluteFilePath(relative));
}

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus, false);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("UPV"));
    QCoreApplication::setApplicationName(QStringLiteral("Proyecto PER"));

    const QString usersPath = dataPath(QStringLiteral("data/users.json"));
    const QString avatarsDir = dataPath(QStringLiteral("data/avatars"));
    const QString problemsPath = dataPath(QStringLiteral("data/problems.json"));

    UserManager userManager(usersPath, avatarsDir);
    if (!userManager.load()) {
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("No se pudo cargar la información de usuarios."));
        return EXIT_FAILURE;
    }

    ProblemManager problemManager(problemsPath);
    if (!problemManager.load()) {
        QMessageBox::critical(nullptr, QObject::tr("Error"), QObject::tr("No se pudieron cargar los problemas disponibles."));
        return EXIT_FAILURE;
    }

    MainWindow window(userManager, problemManager);
    window.show();
    return app.exec();
}
