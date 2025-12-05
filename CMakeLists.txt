cmake_minimum_required(VERSION 3.21)
project(ProyectoPER LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 COMPONENTS Widgets Gui Svg Sql REQUIRED)

qt_standard_project_setup()

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/registerdialog.cpp
    src/profiledialog.cpp
    src/resultsdialog.cpp
    src/usermanager.cpp
    src/problemmanager.cpp
    src/chartscene.cpp
    src/chartview.cpp
    src/protractoritem.cpp
    src/ruleritem.cpp
    src/distanceitem.cpp
)

set(PROJECT_HEADERS
    include/chartscene.h
    include/chartview.h
    include/registerdialog.h
    include/profiledialog.h
    include/resultsdialog.h
    include/mainwindow.h
    include/problemmanager.h
    include/protractoritem.h
    include/ruleritem.h
    include/distanceitem.h
    include/usermanager.h
)

qt_add_executable(ProyectoPER
    ${PROJECT_SOURCES}
    ${PROJECT_HEADERS}
    resources/app_resources.qrc
)

target_include_directories(ProyectoPER PRIVATE
    include
)

target_precompile_headers(ProyectoPER PRIVATE <cstdio>)

target_link_libraries(ProyectoPER PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/libnavlib.a
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
    Qt6::Sql
)

add_custom_command(TARGET ProyectoPER POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/../navdb.sqlite
        $<TARGET_FILE_DIR:ProyectoPER>/navdb.sqlite)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../navdb.sqlite
        DESTINATION ${CMAKE_INSTALL_BINDIR})

install(TARGETS ProyectoPER)
