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
endif ()
