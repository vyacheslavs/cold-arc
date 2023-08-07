cmake_minimum_required(VERSION 3.26)
project(cold_arc_gtk)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(SQLITE3 sqlite3)

link_directories(
        ${GTKMM_LIBRARY_DIRS}
        ${SQLITE3_LIBRARY_DIRS}
)

include_directories(
        ${GTKMM_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
)

set(CMAKE_CXX_STANDARD 17)

add_executable(cold_arc_gtk main.cpp
        MainWindow.cpp
        MainWindow.h
        res/resources.c
        Archive.cpp
        Archive.h
        ArchiveSettingsDialog.cpp
        ArchiveSettingsDialog.h
        Utils.h
        SqLiteHandle.cpp
        SqLiteHandle.h
        Utils.cpp
        NewMediaDialog.cpp
        NewMediaDialog.h
        Signals.cpp
        Signals.h
        Exceptions.h)

target_link_libraries(cold_arc_gtk
        ${GTKMM_LIBRARIES}
        ${SQLITE3_LIBRARIES}
)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/res/resources.c
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/res
        COMMAND glib-compile-resources --target=${PROJECT_SOURCE_DIR}/res/resources.c --generate-source ${PROJECT_SOURCE_DIR}/res/resources.xml
        DEPENDS ${PROJECT_SOURCE_DIR}/res/resources.xml
        DEPENDS ${PROJECT_SOURCE_DIR}/res/glade.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/settings.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/skeleton.db
)

add_custom_target(resources
        DEPENDS skeleton_db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/resources.c
)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/res/skeleton.db
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/res
        COMMAND cat ${PROJECT_SOURCE_DIR}/res/skeleton_db.sql | sqlite3 ${PROJECT_SOURCE_DIR}/res/skeleton.db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/skeleton_db.sql
)

add_custom_target(skeleton_db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/skeleton.db
)

add_dependencies(cold_arc_gtk resources)
