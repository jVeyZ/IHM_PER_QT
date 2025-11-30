QT += widgets gui svg sql
CONFIG += c++20
TEMPLATE = app
TARGET = ProyectoPER

INCLUDEPATH += $$PWD/include

LIBS += -L$$PWD/lib -lnavlib

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/registerdialog.cpp \
    src/profiledialog.cpp \
    src/resultsdialog.cpp \
    src/usermanager.cpp \
    src/problemmanager.cpp \
    src/chartscene.cpp \
    src/chartview.cpp \
    src/protractoritem.cpp \
    src/ruleritem.cpp \
    src/distanceitem.cpp

HEADERS += \
    include/chartscene.h \
    include/chartview.h \
    include/registerdialog.h \
    include/profiledialog.h \
    include/resultsdialog.h \
    include/mainwindow.h \
    include/problemmanager.h \
    include/protractoritem.h \
    include/ruleritem.h \
    include/distanceitem.h \
    include/usermanager.h

RESOURCES += resources/app_resources.qrc

# Ensure navdb.sqlite travels with the build output.
pwdnavdb = $$PWD/navdb.sqlite
COPIES += navdbcopy
navdbcopy.files = $$pwdnavdb
navdbcopy.path = $$OUT_PWD
