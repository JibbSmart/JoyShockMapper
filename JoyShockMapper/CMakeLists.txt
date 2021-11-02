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
 	src/DigitalButton.cpp
	src/MotionImpl.cpp
	src/Mapping.cpp
    src/TriggerEffectGenerator.cpp
    include/TriggerEffectGenerator.h
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
    include/MotionIf.h
	include/Gamepad.h
	include/DigitalButton.h
	include/JslWrapper.h
	include/Mapping.h
)

if (WINDOWS)
	if(SDL)
		target_sources (
				${BINARY_NAME} PRIVATE
				src/SDL2Wrapper.cpp
		)
		add_definitions(-DSDL2)
	else()
		target_sources (
				${BINARY_NAME} PRIVATE
				src/JslWrapper.cpp
		)
	endif()

    target_sources (
        ${BINARY_NAME} PRIVATE
        src/win32/InputHelpers.cpp
        src/win32/PlatformDefinitions.cpp
        src/win32/WindowsTrayIcon.cpp        include/win32/WindowsTrayIcon.h
        src/win32/Gamepad.cpp
        src/win32/HidHideApi.cpp             include/HidHideApi.h
        src/win32/HidHideWhitelister.cpp
        "Win32 Dialog.rc"                    include/win32/resource.h
    )

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
		 VS_DEBUGGER_COMMAND_ARGUMENTS "$(SolutionDir)../dist"
    )

	 add_definitions(/bigobj)
endif ()

if (LINUX)
	if(SDL OR NOT DEFINED SDL)
		target_sources (
				${BINARY_NAME} PRIVATE
				src/SDL2Wrapper.cpp
		)
		add_definitions(-DSDL2)
	else()
		target_sources (
				${BINARY_NAME} PRIVATE
				src/JslWrapper.cpp
		)
	endif()

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

if(SDL OR NOT DEFINED SDL)
	# SDL2
	set(HIDAPI ON)
	CPMAddPackage (
		NAME SDL2
		GITHUB_REPOSITORY libsdl-org/SDL
		GIT_TAG 3b7649c578b1ff98fb3f1c76fc1a586fe75fb617
	)
	
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


	target_link_libraries (
		${BINARY_NAME} PRIVATE
		Platform::Dependencies
		SDL2
	)

	install (
		TARGETS ${BINARY_NAME} SDL2
		RUNTIME DESTINATION ${PACKAGE_DIR}
	)
else()
	# JoyShockLibrary
	CPMAddPackage (
		NAME JoyShockLibrary
		GITHUB_REPOSITORY JibbSmart/JoyShockLibrary
		GIT_TAG 0
	)

	target_link_libraries (
		${BINARY_NAME} PRIVATE
		Platform::Dependencies
		JoyShockLibrary
	)

	install (
		TARGETS ${BINARY_NAME} JoyShockLibrary
		RUNTIME DESTINATION ${PACKAGE_DIR}
	)
endif()


# magic_enum
CPMAddPackage (
    NAME magic_enum
    GITHUB_REPOSITORY jamek/magic_enum
    GIT_TAG 47e34ada93e0bf70dcea551636755cd66d893768
)

target_link_libraries (
    ${BINARY_NAME} PRIVATE
    Platform::Dependencies
    magic_enum
)

# pocket_fsm
CPMAddPackage (
    NAME pocket_fsm
    GITHUB_REPOSITORY Electronicks/pocket_fsm
    GIT_TAG e447ec24c7a547bd1fbe8d964baa866a9cf146c8
)

target_link_libraries (
    ${BINARY_NAME} PRIVATE
    Platform::Dependencies
    pocket_fsm
)

# GamepadMotionHelpers
CPMAddPackage (
    NAME GamepadMotionHelpers
    GITHUB_REPOSITORY JibbSmart/GamepadMotionHelpers
    GIT_TAG main
)

target_link_libraries (
    ${BINARY_NAME} PRIVATE
    Platform::Dependencies
    GamepadMotionHelpers
)