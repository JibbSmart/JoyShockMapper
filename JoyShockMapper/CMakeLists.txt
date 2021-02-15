set (BINARY_NAME ${PROJECT_NAME})

git_describe(GIT_TAG --tags --dirty=_d)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" JSM_VERSION_MAJOR "${GIT_TAG}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" JSM_VERSION_MINOR "${GIT_TAG}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" JSM_VERSION_PATCH "${GIT_TAG}")
set(JSM_VERSION "v${JSM_VERSION_MAJOR}.${JSM_VERSION_MINOR}.${JSM_VERSION_PATCH}")

message(STATUS "JSM version is " ${JSM_VERSION})

configure_file(include/JSMVersion.h.in include/JSMVersion.h)

add_executable (
    ${BINARY_NAME}
    src/main.cpp
    src/operators.cpp
    src/CmdRegistry.cpp
    src/quatMaths.cpp
    include/InputHelpers.h
    include/PlatformDefinitions.h
    include/TrayIcon.h
    include/Whitelister.h
    include/CmdRegistry.h
    include/JSMAssignment.hpp
    include/JSMVariable.hpp
    include/Whitelister.h
    include/JoyShockMapper.h
    include/GamepadMotion.hpp
)

if (WINDOWS)
    target_sources (
        ${BINARY_NAME} PRIVATE
        src/win32/InputHelpers.cpp
                                            include/win32/resource.h
        src/win32/PlatformDefinitions.cpp
        src/win32/WindowsTrayIcon.cpp       include/win32/WindowsTrayIcon.h
        src/win32/Whitelister.cpp
        "Win32 Dialog.rc"
    )

    set_target_properties (
        ${BINARY_NAME}
        PROPERTIES WIN32_EXECUTABLE ON
    )

    set_target_properties (
        ${BINARY_NAME}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
        "$(ProjectDir)/$(Configuration)/"
    )

    set (BUILD_SHARED_LIBS ON)

    add_custom_command(
        TARGET ${BINARY_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:SDL2>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:SDL2>"
    )

	 add_definitions(/bigobj)
endif ()

if (LINUX)
    target_sources (
        ${BINARY_NAME} PRIVATE
        src/linux/Init.cpp
        src/linux/InputHelpers.cpp
        src/linux/PlatformDefinitions.cpp
        src/linux/StatusNotifierItem.cpp    include/linux/StatusNotifierItem.h
        src/linux/Whitelister.cpp
    )
endif ()

target_compile_definitions (
    ${BINARY_NAME} PRIVATE
    -DAPPLICATION_NAME="JoyShockMapper"
    -DAPPLICATION_RDN="com.github."
)

target_include_directories (
    ${BINARY_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}/include"
)

CPMAddPackage (
    NAME SDL2
    GITHUB_REPOSITORY libsdl-org/SDL
    VERSION 2.0.15
    # 2.0.15 hasn't released yet, so let's use a specific commit
    GIT_TAG a2fbc452ca7cf9c5839e009ce111552f8db38e89
)

target_link_libraries (
    ${BINARY_NAME} PRIVATE
    Platform::Dependencies
    SDL2
)

install (
    TARGETS ${BINARY_NAME} SDL2
)

CPMAddPackage (
    NAME magic_enum
    GITHUB_REPOSITORY Neargye/magic_enum
    GIT_TAG v0.6.6
)

target_link_libraries (
    ${BINARY_NAME} PRIVATE
    Platform::Dependencies
    magic_enum
)
