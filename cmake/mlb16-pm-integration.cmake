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

# TASK-020: one bounded startup-to-state-dispatch observation, no DXT internals.
option(MLB16_TASK020_DIAGNOSTIC "Observe boot progress before DXT candidate state" OFF)
if(MLB16_TASK020_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC OR MLB16_TASK019_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/dxtjob_boot_diagnostic.cpp")
    foreach(symbol IN ITEMS _Z13func_00011CE8P11ppu_context _Z13func_000136F8P11ppu_context)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()

# TASK-021B: one bounded validation observer, with original guest calls preserved.
option(MLB16_TASK021B_DIAGNOSTIC "Observe guest WAD-length validation and error edge" OFF)
if(MLB16_TASK021B_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC OR MLB16_TASK019_DIAGNOSTIC OR MLB16_TASK020_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/wad_validation_diagnostic.cpp")
    foreach(symbol IN ITEMS
        _Z13func_00016D54P11ppu_context
        _Z13func_0007C174P11ppu_context
        _Z13func_00076BACP11ppu_context
        _Z13func_00F2610CP11ppu_context
        open)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()

# TASK-022 reuses the validation observer with only startup/dispatch counters.
option(MLB16_TASK022_DIAGNOSTIC "Compare protected alternate HDD validation and boot progress" OFF)
if(MLB16_TASK022_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC OR MLB16_TASK019_DIAGNOSTIC OR MLB16_TASK020_DIAGNOSTIC OR MLB16_TASK021B_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/wad_validation_diagnostic.cpp")
    set_source_files_properties("${MLB16_PORT_DIR}/src/wad_validation_diagnostic.cpp" PROPERTIES
        COMPILE_DEFINITIONS MLB16_TASK022_DIAGNOSTIC=1)
    foreach(symbol IN ITEMS
        _Z13func_00016D54P11ppu_context
        _Z13func_0007C174P11ppu_context
        _Z13func_00F2610CP11ppu_context
        _Z13func_00011CE8P11ppu_context
        _Z13func_000136F8P11ppu_context
        open)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()

option(MLB16_TASK022R_DIAGNOSTIC "Observe isolated writable validation and boot progress" OFF)
if(MLB16_TASK022R_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC OR MLB16_TASK019_DIAGNOSTIC OR MLB16_TASK020_DIAGNOSTIC OR MLB16_TASK021B_DIAGNOSTIC OR MLB16_TASK022_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/wad_validation_diagnostic.cpp")
    set_source_files_properties("${MLB16_PORT_DIR}/src/wad_validation_diagnostic.cpp" PROPERTIES
        COMPILE_DEFINITIONS "MLB16_TASK022_DIAGNOSTIC=1;MLB16_TASK022R_DIAGNOSTIC=1")
    foreach(symbol IN ITEMS
        _Z13func_00016D54P11ppu_context _Z13func_0007C174P11ppu_context
        _Z13func_00F2610CP11ppu_context _Z13func_00011CE8P11ppu_context
        _Z13func_000136F8P11ppu_context _Z13func_000765E4P11ppu_context open)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()

option(MLB16_TASK023_DIAGNOSTIC "Observe prepare-flip import and synchronous callback returns" OFF)
if(MLB16_TASK023_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC OR MLB16_TASK019_DIAGNOSTIC OR MLB16_TASK020_DIAGNOSTIC OR MLB16_TASK021B_DIAGNOSTIC OR MLB16_TASK022_DIAGNOSTIC OR MLB16_TASK022R_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/flip_boundary_diagnostic.cpp")
    foreach(symbol IN ITEMS
        _Z13func_00066530P11ppu_context _Z13func_00F25BCCP11ppu_context
        _Z13func_00011CE8P11ppu_context _Z13func_000136F8P11ppu_context ppu_guest_call)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()

# TASK-024: dispatcher/caller and immediate continuation only.
option(MLB16_TASK024_DIAGNOSTIC "Observe dispatcher progress and immediate caller continuation" OFF)
if(MLB16_TASK024_DIAGNOSTIC)
    if(MLB16_TASK018_DIAGNOSTIC OR MLB16_TASK019_DIAGNOSTIC OR MLB16_TASK020_DIAGNOSTIC OR MLB16_TASK021B_DIAGNOSTIC OR MLB16_TASK022_DIAGNOSTIC OR MLB16_TASK022R_DIAGNOSTIC OR MLB16_TASK023_DIAGNOSTIC)
        message(FATAL_ERROR "Select only one task diagnostic")
    endif()
    target_sources(${PROJECT_NAME} PRIVATE "${MLB16_PORT_DIR}/src/dispatcher_progress_diagnostic.cpp")
    foreach(symbol IN ITEMS
        _Z13func_00011CE8P11ppu_context _Z13func_000136F8P11ppu_context
        _Z13func_00014EB4P11ppu_context ps3_indirect_call)
        target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
endif()
