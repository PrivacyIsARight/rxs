if (WITH_DMI AND (RXS_OS_LINUX OR RXS_OS_FREEBSD))
    set(WITH_DMI ON)
else()
    set(WITH_DMI OFF)
endif()

if (WITH_DMI)
    add_definitions(/DRXS_FEATURE_DMI)

    list(APPEND HEADERS
        src/hw/dmi/DmiBoard.h
        src/hw/dmi/DmiMemory.h
        src/hw/dmi/DmiReader.h
        src/hw/dmi/DmiTools.h
        )

    list(APPEND SOURCES
        src/hw/dmi/DmiBoard.cpp
        src/hw/dmi/DmiMemory.cpp
        src/hw/dmi/DmiReader.cpp
        src/hw/dmi/DmiTools.cpp
        )

    list(APPEND SOURCES src/hw/dmi/DmiReader_unix.cpp)
else()
    remove_definitions(/DRXS_FEATURE_DMI)
endif()
