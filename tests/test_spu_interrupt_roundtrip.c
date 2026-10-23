/* TASK-005: original synthetic interrupt round-trip, with no game inputs.
 * Catches a missing vector dispatch, lost event/ack, broken SRR0/register
 * restoration, or an interrupt repeating after acknowledgment.
 *
 * Reuse the upstream CPU-only fixture's context and fail-fast external link
 * boundaries. Channels, DMA list producer, interrupt hook, resolver, drain,
 * and register restoration are the real runtime. Synthetic callbacks follow
 * the lifted-function ABI; this does not test the lifter or a game PM.
 *
 * PowerShell build from this repository (outputs remain on F:):
 * $sdk = 'F:/Games/MLB16PS3/ps3recomp-source'
 * $spu = "$sdk/runtime/spu"
 * $src = 'spu_channels.c','spu_drain.c','spu_lockstep.c','spu_coherency.c',
 *        'spu_workload.c','spurs_policy.c','spurs_job.c','spu_tsp_weak.c',
 *        'spu_vm_pagemap.c','spurs_policy_blob_weak.c','spu_interp.c' |
 *        ForEach-Object { "$spu/$_" }
 * $env:PATH = 'D:/APP/msys64/clang64/bin;' + $env:PATH
 * $env:TEMP = $env:TMP = 'F:/Games/MLB16PS3/tmp/task005-interrupt'
 * New-Item -ItemType Directory -Force $env:TEMP | Out-Null
 * & D:/APP/msys64/clang64/bin/clang.exe -std=gnu17 -O1 -UNDEBUG `
 *   "-I$sdk/include" "-I$spu" "-I$spu/tests" "-I$sdk/runtime/platform" `
 *   tests/test_spu_interrupt_roundtrip.c $src `
 *   "$sdk/runtime/platform/win32_compat.c" -pthread -lm `
 *   -o "$env:TEMP/interrupt-roundtrip.exe"
 * & "$env:TEMP/interrupt-roundtrip.exe"
 * Run with SPU_/SPURS_/YDKJ_/LBP_/GH3_ diagnostic overrides unset.
 */
#define main upstream_machine_status_main
#include "test_spu_machine_status.c"
#undef main

extern void spu_wrch(spu_context*, uint32_t, u128);
extern int spu_run_with_halt(void (*)(spu_context*), spu_context*);
extern void spu_halt(spu_context*);

enum { HANDLER = 0xA2C, CONTINUATION = 0x200, SECOND_HOP = 0x204,
       LIST = 0x100, TAG = 5, EVENT = 2 };
static unsigned handler_hits, continuation_hits, second_hits;
static u128 before_irq[128];

static void channel(spu_context* c, uint32_t ch, uint32_t value)
{
    spu_wrch(c, ch, spu_pref_u32(value));
}

static void interrupt_handler(spu_context* c)
{
    assert(++handler_hits == 1);
    assert(c->pc == HANDLER && c->srr0 == CONTINUATION);
    assert(c->int_enable == 0 && c->irq_saved == 1);
    assert(spu_rdch(c, SPU_RdEventStat)._u32[0] == EVENT);
    assert(spu_rdch(c, MFC_RdListStallStat)._u32[0] == (1u << TAG));
    channel(c, MFC_WrListStallAck, TAG);
    channel(c, SPU_WrEventAck, EVENT);
    assert((c->event_status & c->event_mask) == 0);
    assert(c->list_stall_mask == 0 && c->list_stall_stat == 0);

    /* Deliberately clobber every GPR: only the real interrupt-return path
     * may restore them. This is the C lowering of an interrupt-enabled iret
     * (tools/spu_lifter.py), not a hand-written register restore. */
    memset(c->gpr, 0xA5, sizeof c->gpr);
    c->int_enable = 1;
    c->pc = c->srr0;
    g_spu_trampoline_fn = spu_indirect_branch;
}

static void continued(spu_context* c)
{
    assert(++continuation_hits == 1 && handler_hits == 1);
    assert(c->pc == CONTINUATION && c->srr0 == CONTINUATION);
    assert(c->int_enable == 1 && c->irq_saved == 0);
    assert(memcmp(c->gpr, before_irq, sizeof before_irq) == 0);
    assert(spu_rchcnt(c, SPU_RdEventStat) == 0);
    c->gpr[10]._u32[0] = 0x12345678;
    c->pc = SECOND_HOP;
    g_spu_trampoline_fn = spu_indirect_branch;
}

static void second_hop(spu_context* c)
{
    assert(++second_hits == 1 && handler_hits == 1 && continuation_hits == 1);
    assert(c->pc == SECOND_HOP && c->srr0 == CONTINUATION);
    assert(c->int_enable == 1 && c->irq_saved == 0);
    assert(c->event_status == 0 && c->event_mask == EVENT);
    assert(c->gpr[10]._u32[0] == 0x12345678);
    c->status = SPU_STATUS_STOPPED_BY_STOP;
    spu_halt(c);
}

static void arm_event(spu_context* c)
{
    /* Original ISA encoding: bra HANDLER at LS zero, installed with the
     * same quadword store helper used by lifted stqa. */
    spu_ls_write128(c, 0, spu_splat_u32(0x30000000u | ((HANDLER >> 2) << 7)));
    channel(c, SPU_WrEventMask, EVENT);
    c->int_enable = 1;
    assert(c->event_status == 0);

    /* One list element: stall-and-notify, zero transfer bytes. This raises
     * the real DMA event without accessing guest memory or mocking events. */
    word(LIST, 0x80000000u);
    word(LIST + 4, 0);
    channel(c, MFC_LSA, 0x1000);
    channel(c, MFC_EAH, 0);
    channel(c, MFC_EAL, LIST);
    channel(c, MFC_Size, 8);
    channel(c, MFC_TagID, TAG);
    channel(c, MFC_Cmd, 0x44); /* GETL */
    assert(c->event_status == EVENT && c->list_stall_mask == (1u << TAG));
    assert(handler_hits == 0);
    c->pc = CONTINUATION;
    g_spu_trampoline_fn = spu_indirect_branch;
}

int main(void)
{
    spu_context_init(&ctx, 0);
    ctx.image_id = 77;
    ctx.policy_mode = 1; /* Exercise PM-mode dispatch, not job completion at LS0. */
    ctx.status = SPU_STATUS_RUNNING;
    for (unsigned reg = 0; reg < 128; ++reg)
        for (unsigned lane = 0; lane < 4; ++lane)
            ctx.gpr[reg]._u32[lane] = 0x10000000u + reg * 16 + lane;
    memcpy(before_irq, ctx.gpr, sizeof before_irq);
    spu_begin_image(77);
    spu_register_function(HANDLER, interrupt_handler);
    spu_register_function(CONTINUATION, continued);
    spu_register_function(SECOND_HOP, second_hop);

    int halted = spu_run_with_halt(arm_event, &ctx);
    assert(halted == 1 && ctx.status == SPU_STATUS_STOPPED_BY_STOP);
    assert(handler_hits == 1 && continuation_hits == 1 && second_hits == 1);
    assert(ctx.pc == SECOND_HOP && ctx.srr0 == CONTINUATION);
    assert(ctx.event_status == 0 && ctx.list_stall_mask == 0);
    assert(g_spu_trampoline_fn == NULL);
    puts("PASS: interrupt round-trip: handler=1 continuation=1 second-hop=1 "
         "SRR0=0x200 final-PC=0x204 registers=restored event=acknowledged");
    return 0;
}
