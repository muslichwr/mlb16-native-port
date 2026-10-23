/* TASK-011 experiment. Link this object with -Wl,--wrap=main and the unchanged
 * TASK-009 PM object. MinGW CRT's main reference resolves to __wrap_main;
 * __real_main resolves to the existing runner. No constructor or runner patch.
 * This file contains original registration glue only, not lifted game code.
 */
#include "spu_workload.h"
#include <stdio.h>

extern int __real_main(int argc, char** argv);
extern void spu_begin_image(int image_id);
extern spu_lifted_entry_fn spu_lookup(uint32_t addr, int image_id);
extern void mlb16_pm_spu_recomp_register(void);
extern void mlb16_pm_spu_func_00000A00(spu_context*);
extern void mlb16_pm_spu_func_00000A2C(spu_context*);
extern void mlb16_pm_spu_func_00002B80(spu_context*);

int __wrap_main(int argc, char** argv)
{
    const int image_id = 77;
    const uint64_t fp = UINT64_C(0x75BFE68AE96D7577);
    int found_id = -1;
    /* Reject occupied image IDs (including wildcard matches) or fingerprints
     * instead of overwriting an existing registration. No guest is running. */
    for (uint32_t pc = 0; pc < SPU_LS_SIZE; pc += 4) {
        if (spu_lookup(pc, image_id)) {
            fprintf(stderr, "[task011] image 77 is occupied; boot refused\n");
            return 3;
        }
    }
    if (spu_workload_find_img(fp, &found_id)) {
        fprintf(stderr, "[task011] fingerprint already registered; boot refused\n");
        return 3;
    }
    spu_begin_image(image_id);
    mlb16_pm_spu_recomp_register();
    spu_begin_image(0);
    spu_workload_register_img(fp, mlb16_pm_spu_func_00000A00,
                              image_id, "mlb16 raw PM diagnostic");
    if (spu_workload_find_img(fp, &found_id) != mlb16_pm_spu_func_00000A00 ||
        found_id != image_id ||
        spu_lookup(0xA00, image_id) != mlb16_pm_spu_func_00000A00 ||
        spu_lookup(0xA2C, image_id) != mlb16_pm_spu_func_00000A2C ||
        spu_lookup(0x2B80, image_id) != mlb16_pm_spu_func_00002B80) {
        fprintf(stderr, "[task011] registration verification failed; boot refused\n");
        return 3;
    }
    fprintf(stderr, "[task011] READY fp=0x75BFE68AE96D7577 image=77 "
                    "entry=0xA00 handler=0xA2C continuation=0x2B80; "
                    "verified before runner main\n");
    fflush(stderr);
    return __real_main(argc, argv);
}
