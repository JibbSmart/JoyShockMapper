if (UNIX AND NOT APPLE)
    set (LINUX ON)

	find_package (PkgConfig QUIET REQUIRED)

	pkg_search_module (Gtkmm REQUIRED IMPORTED_TARGET gtkmm-3.0)
	pkg_search_module (appindicator REQUIRED IMPORTED_TARGET appindicator3-0.1)
	pkg_search_module (libinput REQUIRED IMPORTED_TARGET libinput)
	pkg_search_module (evdev REQUIRED IMPORTED_TARGET libevdev)

	add_library (
		platform_dependencies INTERFACE
	)

	target_link_libraries (
		platform_dependencies INTERFACE
		PkgConfig::Gtkmm
		PkgConfig::appindicator
		PkgConfig::evdev
        pthread
        dl
    )

	add_library (Platform::Dependencies ALIAS platform_dependencies)
	
    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/50-joyshockmapper.rules
        DESTINATION lib/udev/rules.d
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/JoyShockMapper/gyro_icon.png
        DESTINATION share/icons/hicolor/512x512/apps
        RENAME JoyShockMapper.png
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/JoyShockMapper.desktop
        DESTINATION share/applications
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status.svg
        DESTINATION share/icons/hicolor/16x16/status
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status-dark.svg
        DESTINATION share/icons/hicolor/16x16/status
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status.svg
        DESTINATION share/icons/hicolor/22x22/status
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status-dark.svg
        DESTINATION share/icons/hicolor/22x22/status
    )
    
    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status.svg
        DESTINATION share/icons/hicolor/24x24/status
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status-dark.svg
        DESTINATION share/icons/hicolor/24x24/status
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status.svg
        DESTINATION share/icons/hicolor/scalable/status
    )

    install (
        FILES ${PROJECT_SOURCE_DIR}/dist/linux/jsm-status-dark.svg
        DESTINATION share/icons/hicolor/scalable/status
    )
endif ()
