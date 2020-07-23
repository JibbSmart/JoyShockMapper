cmake_minimum_required (VERSION 3.12)

project ("JoyShockMapper" LANGUAGES CXX VERSION 2.0.0)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include (cmake/LinuxConfig.cmake)
include (cmake/WindowsConfig.cmake)
include (cmake/CPM.cmake)

add_subdirectory (JoyShockMapper)
