cmake_minimum_required(VERSION 3.30)
project(rxs VERSION 0.0.1 LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_HWLOC           "Enable hwloc support" ON)
option(WITH_RANDOMX         "Enable RandomX algorithms family" ON)
option(WITH_HTTP            "Enable HTTP protocol support (client/server)" ON)
option(WITH_DEBUG_LOG       "Enable debug log output" OFF)
option(WITH_TLS             "Enable OpenSSL support" ON)
option(WITH_ASM             "Enable ASM PoW implementations" ON)
option(WITH_MSR             "Enable MSR mod & 1st-gen Ryzen fix" ON)
option(WITH_ENV_VARS        "Enable environment variables support in config file" ON)
option(WITH_EMBEDDED_CONFIG "Enable internal embedded JSON config" OFF)
option(WITH_PROFILING       "Enable profiling for developers" OFF)
option(WITH_SSE4_1          "Enable SSE 4.1 for Blake2" ON)
option(WITH_AVX2            "Enable AVX2 for Blake2" ON)
option(WITH_VAES            "Enable VAES (AVX-512 + VAES) support" ON)
option(WITH_BENCHMARK       "Enable builtin RandomX benchmark and stress test" ON)
option(WITH_SECURE_JIT      "Enable secure access to JIT memory" OFF)
option(WITH_DMI             "Enable DMI/SMBIOS reader" ON)
option(ENABLE_MARCH_NATIVE  "Tune the binary for the build machine's CPU" ON)

option(BUILD_STATIC         "Build static binary" OFF)
option(ARM_V8               "Force ARMv8 (64 bit) architecture, use with caution if automatic detection fails, but you sure it may work" OFF)
option(ARM_V7               "Force ARMv7 (32 bit) architecture, use with caution if automatic detection fails, but you sure it may work" OFF)
option(HWLOC_DEBUG          "Enable hwloc debug helpers and log" OFF)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


include (CheckIncludeFile)
include (cmake/cpu.cmake)
include (cmake/os.cmake)
include (src/base/base.cmake)
include (src/backend/backend.cmake)


set(HEADERS
    "${HEADERS_BASE}"
    "${HEADERS_BASE_HTTP}"
    "${HEADERS_BACKEND}"
    src/App.h
    src/core/config/Config_default.h
    src/core/config/Config_platform.h
    src/core/config/Config.h
    src/core/config/ConfigTransform.h
    src/core/config/usage.h
    src/core/Controller.h
    src/core/Miner.h
    src/net/interfaces/IJobResultListener.h
    src/net/JobResult.h
    src/net/JobResults.h
    src/net/Network.h
    src/Summary.h
    src/version.h
   )

set(HEADERS_CRYPTO
    src/crypto/common/HugePagesInfo.h
    src/crypto/common/MemoryPool.h
    src/crypto/common/Nonce.h
    src/crypto/common/portable/mm_malloc.h
    src/crypto/common/VirtualMemory.h
   )

set(SOURCES
    "${SOURCES_BASE}"
    "${SOURCES_BASE_HTTP}"
    "${SOURCES_BACKEND}"
    src/App.cpp
    src/core/config/Config.cpp
    src/core/config/ConfigTransform.cpp
    src/core/Controller.cpp
    src/core/Miner.cpp
    src/net/JobResults.cpp
    src/net/Network.cpp
    src/Summary.cpp
    src/rxs.cpp
   )

set(SOURCES_CRYPTO
    src/crypto/common/HugePagesInfo.cpp
    src/crypto/common/MemoryPool.cpp
    src/crypto/common/Nonce.cpp
    src/crypto/common/VirtualMemory.cpp
   )

if (WITH_HWLOC)
    list(APPEND HEADERS_CRYPTO
        src/crypto/common/NUMAMemoryPool.h
        )

    list(APPEND SOURCES_CRYPTO
        src/crypto/common/NUMAMemoryPool.cpp
        src/crypto/common/VirtualMemory_hwloc.cpp
        )
endif()

list(APPEND SOURCES_OS src/App_unix.cpp src/crypto/common/VirtualMemory_unix.cpp)

if (RXS_OS_APPLE)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(CORESERVICES_LIBRARY CoreServices REQUIRED)
    set(EXTRA_LIBS ${IOKIT_LIBRARY} ${CORESERVICES_LIBRARY})
elseif (RXS_OS_ANDROID)
    set(EXTRA_LIBS pthread rt dl log)
elseif (RXS_OS_LINUX)
    list(APPEND SOURCES_OS
        src/crypto/common/LinuxMemory.h
        src/crypto/common/LinuxMemory.cpp
        )
    set(EXTRA_LIBS pthread rt dl)
elseif (RXS_OS_FREEBSD)
    set(EXTRA_LIBS kvm pthread)
else()
    set(EXTRA_LIBS pthread)
endif()

add_definitions(-DRXS_MINER_PROJECT -DRXS_JSON_SINGLE_LINE_ARRAY)
add_definitions(-D__STDC_FORMAT_MACROS -DUNICODE -D_FILE_OFFSET_BITS=64)

find_package(UV REQUIRED)

include(cmake/flags.cmake)
include(cmake/randomx.cmake)
include(cmake/argon2.cmake)
include(cmake/OpenSSL.cmake)
include(cmake/asm.cmake)
if (VAES_SUPPORTED)
    set_property(SOURCE src/crypto/randomx/aes_hash.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -mvaes")
endif()

if (WITH_EMBEDDED_CONFIG)
    add_definitions(/DRXS_FEATURE_EMBEDDED_CONFIG)
endif()

include(src/hw/api/api.cmake)
include(src/hw/dmi/dmi.cmake)

include_directories(src)
include_directories(src/3rdparty)
include_directories(${UV_INCLUDE_DIR})

if (WITH_DEBUG_LOG)
    add_definitions(/DAPP_DEBUG)
endif()

if (APPLE)
    add_executable(${CMAKE_PROJECT_NAME} MACOSX_BUNDLE ${HEADERS} ${SOURCES} ${SOURCES_OS} ${HEADERS_CRYPTO} ${SOURCES_CRYPTO} ${SOURCES_SYSLOG} ${TLS_SOURCES} ${RXS_ASM_SOURCES})
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "rxs"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.privacyisaright.rxs"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/macos/Info.plist.in"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
else()
    add_executable(${CMAKE_PROJECT_NAME} ${HEADERS} ${SOURCES} ${SOURCES_OS} ${HEADERS_CRYPTO} ${SOURCES_CRYPTO} ${SOURCES_SYSLOG} ${TLS_SOURCES} ${RXS_ASM_SOURCES})
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} ${RXS_ASM_LIBRARY} ${OPENSSL_LIBRARIES} ${UV_LIBRARIES} ${EXTRA_LIBS} ${CPUID_LIB} ${ARGON2_LIBRARY})

if (NOT WITH_EMBEDDED_CONFIG)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/config.json"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/config.json"
        COMMENT "Copying maximum-throughput configuration"
    )
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
if(ipo_supported)
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES Clang AND CMAKE_BUILD_TYPE STREQUAL Release AND NOT CMAKE_GENERATOR STREQUAL Xcode)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>")
endif()

if (APPLE)
    configure_file("${CMAKE_SOURCE_DIR}/cmake/macos/rxs-launcher.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/rxs" @ONLY)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename
            "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/rxs-bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/rxs"
            "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/rxs"
        COMMAND /bin/chmod +x "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/rxs"
        COMMENT "Installing macOS app launcher"
    )
endif()
