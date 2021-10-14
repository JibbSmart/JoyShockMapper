cmake_minimum_required (VERSION 3.12)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project ("JoyShockMapper" CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include (cmake/LinuxConfig.cmake)
include (cmake/WindowsConfig.cmake)
include (cmake/CPM.cmake)
include (cmake/GetGitRevisionDescription.cmake)

add_subdirectory (JoyShockMapper)
