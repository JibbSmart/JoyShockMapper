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
endif ()
