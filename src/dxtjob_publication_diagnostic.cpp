/* TASK-018: original observation glue. Opt-in at configure AND process start.
 * --wrap observes unresolved inter-object references only. It does not cover
 * calls resolved inside a PPU object; the report must preserve that limit.
 * All original calls receive the original arguments exactly once.
 */
#include "ppu_context.h"
#include "spu_context.h"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

extern "C" uint8_t* vm_base;
extern "C" void __real_spu_wrch(spu_context*, uint32_t, u128);
extern "C" int __real_spu_run_with_halt(void (*)(spu_context*), spu_context*);
#define PPU_REAL(id) extern "C" void __real__Z13func_##id##P11ppu_context(ppu_context*)
PPU_REAL(009FAAB4);
PPU_REAL(009F98B8);
PPU_REAL(009FA818);
PPU_REAL(009FA71C);

namespace {
std::atomic<uint32_t> control{0}, workload{0};
std::atomic<unsigned> appends{0}, resets{0}, runs{0}, fetches{0};
bool enabled() {
    static const bool on = [] { const char* e = std::getenv("MLB16_TASK018_TRACE");
        return e && std::strcmp(e, "1") == 0; }();
    return on;
}
uint32_t be32(const uint8_t* p) {
    uint32_t x;
    if ((reinterpret_cast<uintptr_t>(p) & 3) == 0)
        x = __atomic_load_n(reinterpret_cast<const uint32_t*>(p), __ATOMIC_RELAXED);
    else std::memcpy(&x, p, sizeof x);
    return __builtin_bswap32(x);
}
uint32_t ram(uint32_t ea) { return be32(vm_base + ea); }
void log(const char* fmt, ...) {
    char line[1536];
    va_list ap; va_start(ap, fmt); std::vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    _lock_file(stdout); _lock_file(stderr);
    std::fprintf(stderr, "[task018] %s\n", line); std::fflush(stderr);
    _unlock_file(stderr); _unlock_file(stdout);
}
bool dxt_control(uint32_t ea) { return ea && ea == control.load(); }
bool dxt_spu(spu_context* ctx) {
    const uint32_t w = workload.load();
    return w && ctx->image_id == 77 && be32(ctx->ls + 0x13F0) == w;
}
void summary() {
    std::this_thread::sleep_for(std::chrono::seconds(35));
    log("WINDOW_35S control=%08X workload=%08X append_calls=%u reset_calls=%u pm_runs=%u descriptor_fetches=%u coverage=inter_object_only",
        control.load(), workload.load(), appends.load(), resets.load(), runs.load(), fetches.load());
}
}

extern "C" void __wrap__Z13func_009FAAB4P11ppu_context(ppu_context* ctx) {
    const uint32_t c = static_cast<uint32_t>(ctx->gpr[3]);
    const uint32_t w = static_cast<uint32_t>(ctx->gpr[4]);
    const uint32_t name = static_cast<uint32_t>(ctx->gpr[6]);
    const bool watch = enabled() && vm_base && name == 0x00F2E974 &&
                       std::memcmp(vm_base + name, "DXTJob", 7) == 0;
    if (watch) {
        control.store(c); workload.store(w);
        log("INIT_ENTER producer=009FAAB4 caller_lr=%08llX control=%08X workload=%08X allocation_bytes=%llu nameEA=%08X",
            static_cast<unsigned long long>(ctx->lr), c, w,
            static_cast<unsigned long long>(ctx->gpr[5]), name);
    }
    __real__Z13func_009FAAB4P11ppu_context(ctx);
    if (watch) {
        log("INIT_RETURN control=%08X workload=%08X descriptor_base=%08X capacity_minus_one=%08X append_count=%08X cursor_row=%08X,%08X first_descriptor=%08X,%08X",
            c, w, ram(w+0x3C), ram(w+0x2C), ram(c+0x28), ram(w+0x70), ram(w+0x74), ram(w+0x80), ram(w+0x84));
        static std::atomic<bool> started{false};
        if (!started.exchange(true)) std::thread(summary).detach();
    }
}

extern "C" void __wrap__Z13func_009F98B8P11ppu_context(ppu_context* ctx) {
    const uint32_t c = static_cast<uint32_t>(ctx->gpr[3]);
    const bool watch = enabled() && dxt_control(c);
    __real__Z13func_009F98B8P11ppu_context(ctx);
    if (watch) log("REGISTER_RETURN control=%08X workload=%08X spurs=%08X wid=%u descriptor_base=%08X",
        c, ram(c), ram(c+4), ram(c+8), ram(ram(c)+0x3C));
}

extern "C" void __wrap__Z13func_009FA818P11ppu_context(ppu_context* ctx) {
    const uint32_t c = static_cast<uint32_t>(ctx->gpr[4]);
    const bool watch = enabled() && dxt_control(c);
    unsigned n = 0; uint32_t dest = 0, index = 0, old0 = 0, old1 = 0;
    if (watch) {
        n = ++appends;
        if (n <= 3) {
            index = ram(c+0x28); dest = ram(c+0x20) + 8*index;
            old0 = ram(dest); old1 = ram(dest+4);
            log("APPEND_ENTER n=%u producer=009FA818 caller_lr=%08llX control=%08X workload=%08X wid=%u index=%u dest=%08X input=%016llX before=%08X,%08X count_before=%u",
                n, static_cast<unsigned long long>(ctx->lr), c, ram(c), ram(c+8), index, dest,
                static_cast<unsigned long long>(ctx->gpr[5]), old0, old1, index);
        }
    }
    __real__Z13func_009FA818P11ppu_context(ctx);
    if (watch && n <= 3) log("APPEND_RETURN n=%u dest=%08X after=%08X,%08X count_after=%u cursor_row=%08X,%08X",
        n, dest, ram(dest), ram(dest+4), ram(c+0x28), ram(ram(c)+0x70), ram(ram(c)+0x74));
}

extern "C" void __wrap__Z13func_009FA71CP11ppu_context(ppu_context* ctx) {
    const uint32_t c = static_cast<uint32_t>(ctx->gpr[3]);
    const bool watch = enabled() && dxt_control(c);
    const unsigned n = watch ? ++resets : 0;
    if (watch && n <= 3) log("RESET_ENTER n=%u control=%08X count=%u", n, c, ram(c+0x28));
    __real__Z13func_009FA71CP11ppu_context(ctx);
    if (watch && n <= 3) log("RESET_RETURN n=%u control=%08X count=%u", n, c, ram(c+0x28));
}

extern "C" void __wrap_spu_wrch(spu_context* ctx, uint32_t channel, u128 value) {
    const bool watch = enabled() && channel == MFC_Cmd && value._u32[0] == 0x40 &&
                       ctx->mfc_lsa == 0xD00 && dxt_spu(ctx);
    unsigned n = 0; uint32_t ea = 0, off = 0, before0 = 0, before1 = 0;
    if (watch) {
        n = ++fetches;
        if (n <= 3) {
            ea = ctx->mfc_eal; off = ctx->gpr[60]._u32[0] & 127;
            before0 = ram(ea+off); before1 = ram(ea+off+4);
        }
    }
    __real_spu_wrch(ctx, channel, value);
    if (watch && n <= 3) log("FETCH_RETURN n=%u image=%d spu=%u workload=%08X commandEA=%08X ls=00D00 offset=%u descriptorEA=%08X ram_before=%08X,%08X ls_after=%08X,%08X cursor_row=%08X,%08X append_calls=%u",
        n, ctx->image_id, ctx->spu_id, workload.load(), ea, off, ea+off,
        before0, before1, be32(ctx->ls+0xD00+off), be32(ctx->ls+0xD04+off),
        be32(ctx->ls+0xF0), be32(ctx->ls+0xF4), appends.load());
}

extern "C" int __wrap_spu_run_with_halt(void (*entry)(spu_context*), spu_context* ctx) {
    const bool watch = enabled() && ctx->image_id == 77 && workload.load() &&
                       ctx->gpr[4]._u32[1] == workload.load();
    const unsigned n = watch ? ++runs : 0;
    const int result = __real_spu_run_with_halt(entry, ctx);
    if (watch && n <= 3) log("PM_RETURN n=%u image=%d spu=%u workload=%08X halt_pc=%05X index=%08X descriptor=%08X,%08X type=%u loadState=%u nextLoad=%08X load=%08X run=%08X store=%08X append_calls=%u",
        n, ctx->image_id, ctx->spu_id, be32(ctx->ls+0x13F0), static_cast<uint32_t>(ctx->pc)&SPU_LS_MASK,
        be32(ctx->ls+0x1410), be32(ctx->ls+0x1420), be32(ctx->ls+0x1424), be32(ctx->ls+0x1420)&0xFFFF,
        be32(ctx->ls+0x12A0)&0xFFFF, be32(ctx->ls+0x12C0), be32(ctx->ls+0x12D0),
        be32(ctx->ls+0x12E0), be32(ctx->ls+0x12F0), appends.load());
    return result;
}
