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
