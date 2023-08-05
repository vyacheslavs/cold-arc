cmake_minimum_required(VERSION 3.26)
project(cold_arc_gtk)

find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)

link_directories(
        ${GTKMM_LIBRARY_DIRS}  )

include_directories(
        ${GTKMM_INCLUDE_DIRS}  )

set(CMAKE_CXX_STANDARD 17)

add_executable(cold_arc_gtk main.cpp
        MainWindow.cpp
        MainWindow.h)

target_link_libraries(cold_arc_gtk
        ${GTKMM_LIBRARIES}  )
