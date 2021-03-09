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
    src/ButtonHelp.cpp
    include/InputHelpers.h
    include/PlatformDefinitions.h
    include/TrayIcon.h
    include/Whitelister.h
    include/CmdRegistry.h
    include/JSMAssignment.hpp
    include/JSMVariable.hpp
    include/Whitelister.h
    include/JoyShockMapper.h
    include/ColorCodes.h
    include/GamepadMotion.hpp
	include/Gamepad.h
)

if (WINDOWS)
    target_sources (
        ${BINARY_NAME} PRIVATE
        src/win32/InputHelpers.cpp
        src/win32/PlatformDefinitions.cpp
        src/win32/WindowsTrayIcon.cpp       include/win32/WindowsTrayIcon.h
        src/win32/Whitelister.cpp
        src/win32/Gamepad.cpp
        "Win32 Dialog.rc"                    include/win32/resource.h
    )

    if(SDL)
        target_sources (
            ${BINARY_NAME} PRIVATE
            src/JoyShockLibrary.cpp              include/JoyShockLibrary.h
		)
    endif()

    # ViGEmClient
    CPMAddPackage (
        NAME ViGEmClient
        GITHUB_REPOSITORY ViGEm/ViGEmClient
        VERSION 1.16.107.0
    )
    
    include_external_msproject(ViGEmClient 
      "${PROJECT_BINARY_DIR}/_deps/vigemclient-src/src/ViGEmClient.vcxproj")

    set_target_properties(ViGEmClient PROPERTIES
      MAP_IMPORTED_CONFIG_DEBUG DEBUG_LIB
      MAP_IMPORTED_CONFIG_RELEASE RELEASE_LIB
      VS_PLATFORM_TOOLSET "v141"
      IMPORTED_LOCATION "${PROJECT_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}/$(PlatformShortName)/ViGEmClient.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_BINARY_DIR}/_deps/vigemclient-src/include"
    )
    
    add_dependencies(${BINARY_NAME} ViGEmClient)
    
    target_link_libraries (
        ${BINARY_NAME} PRIVATE
         debug "${PROJECT_BINARY_DIR}/lib/debug/$(PlatformShortName)/ViGEmClient.lib"
		 optimized "${PROJECT_BINARY_DIR}/lib/release/$(PlatformShortName)/ViGEmClient.lib"
    )
    
    target_include_directories (
        ${BINARY_NAME} PUBLIC
        "${PROJECT_BINARY_DIR}/_deps/vigemclient-src/include"
    )

    if(SDL)
		add_custom_command(
			TARGET ${BINARY_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
				"$<TARGET_FILE:SDL2>"
				"$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:SDL2>"
		)
    else()
		add_custom_command(
			TARGET ${BINARY_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
				"$<TARGET_FILE:JoyShockLibrary>"
				"$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:JoyShockLibrary>"
		)
    endif()
	
	set_target_properties (
        ${BINARY_NAME} PROPERTIES
         WIN32_EXECUTABLE ON
         VS_STARTUP_PROJECT "${BINARY_NAME}"
         VS_DEBUGGER_WORKING_DIRECTORY "$(ProjectDir)/$(Configuration)/"
		 VS_DEBUGGER_COMMAND_ARGUMENTS "$(SolutionDir)..\\$(ProjectName)\\dist"
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
		src/linux/Gamepad.cpp
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

if(SDL)
	# SDL2
	CPMAddPackage (
		NAME SDL2
		GITHUB_REPOSITORY libsdl-org/SDL
		VERSION 2.0.15
		# 2.0.15 hasn't released yet, so let's use a specific commit
		GIT_TAG c287087fcce57af086c0a52f74fcdb74e9084c55
	)
	
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
	target_link_libraries (
		${BINARY_NAME} PRIVATE
		Platform::Dependencies
		SDL2
	)

	install (
		TARGETS ${BINARY_NAME} SDL2
	)
else()
	# JoyShockLibrary
	CPMAddPackage (
		NAME JoyShockLibrary
		GITHUB_REPOSITORY JibbSmart/JoyShockLibrary
		VERSION 2.1.0
	)

	target_link_libraries (
		${BINARY_NAME} PRIVATE
		Platform::Dependencies
		JoyShockLibrary
	)

	install (
		TARGETS ${BINARY_NAME} JoyShockLibrary
	)
endif()


# magic_enum
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
