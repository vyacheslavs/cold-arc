cmake_minimum_required(VERSION 3.26)
project(cold_arc_gtk)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(SQLITE3 sqlite3)
find_package(OpenSSL REQUIRED)
find_package(Fontconfig)

set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE_DEB OFF)
set(COLD_ARC_DB_VERSION 10)

add_subdirectory("expected")

link_directories(
        ${GTKMM_LIBRARY_DIRS}
        ${SQLITE3_LIBRARY_DIRS}
)

include_directories(
        ${GTKMM_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        sqlite_modern_cpp/hdr
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
        Utils.cpp
        NewMediaDialog.cpp
        NewMediaDialog.h
        Signals.cpp
        Signals.h
        Exceptions.h
        NewFolderDialog.cpp
        NewFolderDialog.h
        FolderModelColumns.h
        UploadChooserDialog.cpp
        UploadChooserDialog.h
        UploadDialog.cpp
        UploadDialog.h
        UploadFilesCollection.h
        UploadStage1Resolving.cpp
        UploadStage1Resolving.h
        UploadFileInfo.cpp
        UploadFileInfo.h
        UploadListColumns.h
        UploadStage2DbUpdate.cpp
        UploadStage2DbUpdate.h
        Dispatcher.cpp
        Dispatcher.h
        ContentsModelColumns.h
        MediaListColumns.h
)

target_compile_definitions(cold_arc_gtk PRIVATE COLD_ARC_DB_VERSION=${COLD_ARC_DB_VERSION})

target_link_libraries(cold_arc_gtk
        ${GTKMM_LIBRARIES}
        ${SQLITE3_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${Fontconfig_LIBRARIES}
        expected
)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/res/resources.c
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/res
        COMMAND glib-compile-resources --target=${PROJECT_SOURCE_DIR}/res/resources.c --generate-source ${PROJECT_SOURCE_DIR}/res/resources.xml
        DEPENDS ${PROJECT_SOURCE_DIR}/res/resources.xml
        DEPENDS ${PROJECT_SOURCE_DIR}/res/glade.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/settings.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/newfolder.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/uploaddialog.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/updlg.glade
        DEPENDS ${PROJECT_SOURCE_DIR}/res/skeleton.db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/awesome-free-solid-900.otf
        DEPENDS ${PROJECT_SOURCE_DIR}/res/awesome-free-regular-400.otf
        DEPENDS ${PROJECT_SOURCE_DIR}/res/awesome-free-brands-regular-400.otf
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-icon.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-check.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-skipped.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-error.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-close-button.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-root.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-folder.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-folder-1.svg
        DEPENDS ${PROJECT_SOURCE_DIR}/res/ca-file.svg
)

add_custom_target(resources
        DEPENDS skeleton_db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/resources.c
)

add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/res/skeleton.db
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/res
        COMMAND cat ${PROJECT_SOURCE_DIR}/res/skeleton_db.sql | sqlite3 ${PROJECT_SOURCE_DIR}/res/skeleton.db
        COMMAND echo "UPDATE db_settings SET version=${COLD_ARC_DB_VERSION}" | sqlite3 ${PROJECT_SOURCE_DIR}/res/skeleton.db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/skeleton_db.sql
)

add_custom_target(skeleton_db
        DEPENDS ${PROJECT_SOURCE_DIR}/res/skeleton.db
)

add_dependencies(cold_arc_gtk resources)

install(TARGETS cold_arc_gtk DESTINATION bin)
install(FILES flatpak/so.ldd.Coldarc.metainfo.xml DESTINATION /app/share/metainfo)
install(FILES flatpak/so.ldd.Coldarc.desktop DESTINATION /app/share/applications)
install(FILES res/ca-icon.svg DESTINATION /app/share/icons/hicolor/scalable/apps RENAME so.ldd.Coldarc.svg)
