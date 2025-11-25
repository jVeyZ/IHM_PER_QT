cmake_minimum_required(VERSION 3.21)
project(ProyectoPER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 COMPONENTS Widgets Gui Svg REQUIRED)

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
)

target_include_directories(ProyectoPER PRIVATE include)

target_precompile_headers(ProyectoPER PRIVATE <cstdio>)

target_link_libraries(ProyectoPER PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Svg
)

qt_add_resources(ProyectoPER app_resources
    PREFIX "/"
    FILES
        resources/images/carta_nautica.png
        resources/images/default_avatar.svg
        resources/images/icon_hand.svg
        resources/images/icon_point.svg
        resources/images/icon_line.svg
        resources/images/icon_arc.svg
        resources/images/icon_text.svg
        resources/images/icon_fullscreen.svg
        resources/images/icon_eraser.svg
        resources/images/icon_clean.svg
        resources/images/icon_select.svg
        resources/images/icon_crosshair.svg
        resources/images/icon_protractor.svg
        resources/images/icon_ruler.svg
        resources/images/icon_extremes.svg
        resources/images/icon_zoom_in.svg
        resources/images/icon_zoom_out.svg
        resources/images/icon_zoom_reset.svg
        resources/images/icon_cross.svg
            resources/images/icon_logout.svg
            resources/images/icon_profile.svg
            resources/images/icon_results.svg
            resources/images/icon_theme.svg
        resources/images/icon_chevron_down_light.svg
        resources/images/icon_chevron_down_dark.svg
        styles/modern_light.qss
        styles/modern_dark.qss
        styles/lightblue.qss
)

install(TARGETS ProyectoPER)
