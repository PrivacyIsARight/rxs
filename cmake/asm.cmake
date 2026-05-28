set(RXS_ASM_SOURCES "")
set(RXS_ASM_LIBRARY "")

if (WITH_ASM AND NOT RXS_ARM AND NOT RXS_RISCV AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DRXS_FEATURE_ASM)
    set(RXS_ASM_SOURCES
        src/crypto/common/Assembly.h
        src/crypto/common/Assembly.cpp
        src/crypto/randomx/jit_compiler_x86_static.S
    )
    set_source_files_properties(src/crypto/randomx/jit_compiler_x86_static.S PROPERTIES LANGUAGE ASM)
else()
    remove_definitions(-DRXS_FEATURE_ASM)
endif()
