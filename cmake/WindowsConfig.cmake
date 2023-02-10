if (WIN32)
    set (WINDOWS ON)
    set_property (GLOBAL PROPERTY USE_FOLDERS ON)
    enable_language (RC)

    if (MSVC)
        # Statically link the runtime libraries
        set (
            MSVC_COMPILE_FLAGS
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
        )
        foreach (FLAG ${MSVC_COMPILE_FLAGS})
            string(REPLACE "/MD" "/MT" ${FLAG} "${${FLAG}}")
        endforeach ()
    endif ()

    add_library (
        platform_dependencies INTERFACE
    )

    target_compile_definitions (
        platform_dependencies INTERFACE
        -DUNICODE
        -D_UNICODE
        -DNOMINMAX
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
    )

    target_link_libraries (
        platform_dependencies INTERFACE
        wsock32
        wininet
        ws2_32
        comctl32
    )

    add_library (Platform::Dependencies ALIAS platform_dependencies)

	set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/${CONFIG}/install")
	
	if(SDL)
		set(PACKAGE_DIR "${PROJECT_NAME}_${CMAKE_GENERATOR_PLATFORM}")
	else()
		set(PACKAGE_DIR "${PROJECT_NAME}_${CMAKE_GENERATOR_PLATFORM}_legacy")
	endif()

    install (
        DIRECTORY ${PROJECT_SOURCE_DIR}/dist/GyroConfigs
        DESTINATION ${PACKAGE_DIR}
    )

	install (
        DIRECTORY ${PROJECT_SOURCE_DIR}/dist/AutoLoad
        DESTINATION ${PACKAGE_DIR}
    )

	install (
        FILES ${PROJECT_SOURCE_DIR}/dist/OnReset.txt
        DESTINATION ${PACKAGE_DIR}
    )

	install (
        FILES ${PROJECT_SOURCE_DIR}/dist/OnStartup.txt
        DESTINATION ${PACKAGE_DIR}
    )
    
	install (
        FILES ${PROJECT_SOURCE_DIR}/dist/ds4.bmp
        DESTINATION ${PACKAGE_DIR}
    )
    
	install (
        FILES ${PROJECT_SOURCE_DIR}/dist/imgui.ini
        DESTINATION ${PACKAGE_DIR}
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/CHANGELOG.md
        DESTINATION ${PACKAGE_DIR}
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/LICENSE.md
        DESTINATION ${PACKAGE_DIR}
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/README.md
        DESTINATION ${PACKAGE_DIR}
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/README_中文.md
        DESTINATION ${PACKAGE_DIR}
    )
endif ()
