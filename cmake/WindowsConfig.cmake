if (WIN32)
    set (WINDOWS ON)
    set_property (GLOBAL PROPERTY USE_FOLDERS ON)
    enable_language (RC)

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
endif ()
