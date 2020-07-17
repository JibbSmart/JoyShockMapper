set (BINARY_NAME ${PROJECT_NAME})

add_executable (
    ${BINARY_NAME}
    src/main.cpp
    include/InputHelpers.h
    include/PlatformDefinitions.h
    include/TrayIcon.h
    include/Whitelister.h
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
            "$<TARGET_FILE:JoyShockLibrary>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:JoyShockLibrary>"
    )
endif ()

if (LINUX)
    target_sources (
        ${BINARY_NAME} PRIVATE
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
)

CPMAddPackage (
    NAME JoyShockLibrary
    GITHUB_REPOSITORY JibbSmart/JoyShockLibrary
    VERSION 1.2.1
)

target_link_libraries (
    ${BINARY_NAME} PRIVATE
    Platform::Dependencies
    JoyShockLibrary
)

install (
    TARGETS ${BINARY_NAME} JoyShockLibrary
)
