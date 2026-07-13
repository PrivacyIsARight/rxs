if (APPLE)
    set(RXS_OS_APPLE ON)
    set(RXS_OS_MACOS ON)
    add_definitions(-DRXS_OS_APPLE -DRXS_OS_MACOS)

    # Apple Silicon requires the hardened W^X JIT API. RandomX toggles write
    # protection while generating code and execute protection while hashing.
    if (RXS_ARM)
        set(WITH_SECURE_JIT ON)
    endif()
else()
    set(RXS_OS_UNIX ON)
    add_definitions(-DRXS_OS_UNIX)
endif()

if (RXS_OS_APPLE)
    # Handled above.
elseif (ANDROID OR CMAKE_SYSTEM_NAME MATCHES "Android")
    set(RXS_OS_ANDROID ON)
    add_definitions(-DRXS_OS_ANDROID)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(RXS_OS_LINUX ON)
    add_definitions(-DRXS_OS_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL FreeBSD OR CMAKE_SYSTEM_NAME STREQUAL DragonFly)
    set(RXS_OS_FREEBSD ON)
    add_definitions(-DRXS_OS_FREEBSD)
elseif (CMAKE_SYSTEM_NAME STREQUAL OpenBSD)
    set(RXS_OS_OPENBSD ON)
    add_definitions(-DRXS_OS_OPENBSD)
endif()

if (WITH_SECURE_JIT)
    add_definitions(-DRXS_SECURE_JIT)
endif()
