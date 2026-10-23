# Durable SPU PM integration for MLB The Show 16 (TASK-017.6)
# Compiles promoted raw PM lifted source, startup registration glue, and enables --wrap=main.

if(NOT DEFINED MLB16_PORT_DIR)
    set(MLB16_PORT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "Path to mlb16-port repository")
endif()

get_filename_component(MLB16_PORT_DIR "${MLB16_PORT_DIR}" ABSOLUTE)

message(STATUS "MLB16 PM Integration: using ${MLB16_PORT_DIR}")

# 1. Target sources: promoted PM lifted source and startup registration glue
target_sources(${PROJECT_NAME} PRIVATE
    "${MLB16_PORT_DIR}/src/spu/image0077/spu_recomp.c"
    "${MLB16_PORT_DIR}/src/spu_pm_startup.c"
)

# 2. Target includes needed for SPU headers:
# - spu/image0077 directory for spu_recomp.h
# - runtime/spu/tests so relative include "../../runtime/spu/spu_helpers.h" resolves cleanly
# - runtime/spu for spu_workload.h used by spu_pm_startup.c
# - runtime/platform for win32_compat.h
target_include_directories(${PROJECT_NAME} PRIVATE
    "${MLB16_PORT_DIR}/src/spu/image0077"
    "${PS3RECOMP_DIR}/runtime/spu/tests"
    "${PS3RECOMP_DIR}/runtime/spu"
    "${PS3RECOMP_DIR}/runtime/platform"
)

# 3. Source-specific properties: -O2 optimization and suppress unused-label warnings in generated SPU code
set_source_files_properties("${MLB16_PORT_DIR}/src/spu/image0077/spu_recomp.c" PROPERTIES
    COMPILE_OPTIONS "-O2;-Wno-unused-label"
)

# 4. Target linker option: MinGW CRT main wrapper
target_link_options(${PROJECT_NAME} PRIVATE
    "-Wl,--wrap=main"
)

# TASK-018: original, opt-in observation glue; no generated PPU/SPU edits.
option(MLB16_TASK018_DIAGNOSTIC "Trace DXTJob publication boundaries" OFF)
if(MLB16_TASK018_DIAGNOSTIC)
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/dxtjob_publication_diagnostic.cpp")
    foreach(symbol IN ITEMS
        _Z13func_009FAAB4P11ppu_context
        _Z13func_009F98B8P11ppu_context
        _Z13func_009FA818P11ppu_context
        _Z13func_009FA71CP11ppu_context
        spu_wrch spu_run_with_halt)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()

# TASK-019: request eligibility only; TASK-018's descriptor/DMA probe stays off.
option(MLB16_TASK019_DIAGNOSTIC "Trace DXTJob request eligibility" OFF)
if(MLB16_TASK019_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/dxtjob_request_diagnostic.cpp")
    foreach(symbol IN ITEMS
        _Z13func_009DDA00P11ppu_context
        _Z13func_0006EBACP11ppu_context
        _Z13func_0006EBF0P11ppu_context
        _Z13func_0006E988P11ppu_context
        _Z13func_00A55540P11ppu_context
        _Z13func_009FA818P11ppu_context
        _Z13func_009FA42CP11ppu_context)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()
