/* TASK-019: original, read-only request observation glue.
 * Build option and MLB16_TASK019_TRACE=1 are both required. Wrappers forward
 * each original call once. Eligibility below is a diagnostic interpretation
 * of entry inputs, never a replacement for the original guest decisions.
 * Verify the actual COFF call relocations in the linked executable: linker
 * coverage must not be inferred solely from the presence of wrapper symbols.
 */
#include "ppu_context.h"
#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

extern "C" uint8_t* vm_base;
#define REAL(id) extern "C" void __real__Z13func_##id##P11ppu_context(ppu_context*)
REAL(009DDA00);
REAL(0006EBAC);
REAL(0006EBF0);
REAL(0006E988);
REAL(00A55540);
REAL(009FA818);
REAL(009FA42C);

namespace {
constexpr uint32_t outer_global = 0x01113A0C;
std::atomic<unsigned> candidates{0}, adapter_ac{0}, adapter_f0{0}, requests{0};
std::atomic<unsigned> builders{0}, appends{0}, writes{0};
std::atomic<unsigned> candidate_adapters{0};
bool enabled() {
    static const bool on = [] { const char* e = std::getenv("MLB16_TASK019_TRACE");
        return e && std::strcmp(e, "1") == 0; }();
    return on;
}
void log(const char* fmt, ...) {
    char line[1536]; va_list ap; va_start(ap, fmt);
    std::vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    _lock_file(stdout); _lock_file(stderr);
    std::fprintf(stderr, "[task019] %s\n", line); std::fflush(stderr);
    _unlock_file(stderr); _unlock_file(stdout);
}
// VirtualQuery avoids faulting on a null, stale or not-yet-mapped observation
// address. A failed read is reported separately from a real guest zero.
bool read32(uint32_t ea, uint32_t& value) {
    if (!vm_base || (ea & 3)) return false;
    const uint8_t* p = vm_base + ea;
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(p, &info, sizeof info) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) ||
        reinterpret_cast<uintptr_t>(p) + 4 >
            reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize) return false;
    value = __builtin_bswap32(__atomic_load_n(reinterpret_cast<const uint32_t*>(p), __ATOMIC_RELAXED));
    return true;
}
struct Pointers { uint32_t outer=0, control=0; bool outer_ok=false, control_ok=false; };
Pointers pointers() {
    Pointers p; p.outer_ok = read32(outer_global, p.outer);
    if (p.outer_ok && p.outer) p.control_ok = read32(p.outer+4, p.control);
    return p;
}
struct Request {
    unsigned id, built=0, appended=0, written=0;
    uint64_t adapter_caller;
    bool emit;
};
thread_local Request* active = nullptr;
thread_local uint64_t adapter_caller = 0;
thread_local unsigned candidate_id = 0;
thread_local unsigned candidate_adapter_count = 0;

// These labels describe only the entry-snapshot predicate. Actual builder and
// append entry counts provide separate execution evidence.
const char* entry_predicate(const ppu_context* c, const Pointers& p) {
    const uint32_t width = static_cast<uint32_t>(c->gpr[5]);
    const uint32_t count = static_cast<uint32_t>(c->gpr[6]);
    if (!p.outer_ok) return "OUTER_UNREADABLE";
    if (!p.outer) return "OUTER_ZERO";
    if (count & 3) return "COUNT_MOD4";
    if (width & 3) return "WIDTH_MOD4";
    const int32_t stride = static_cast<uint32_t>(c->gpr[8]) == 0x0600AAE4
        ? static_cast<int32_t>(width) / 2 : static_cast<int32_t>(width);
    if (stride < 4 && count > 4) return "SMALL_STRIDE_COUNT_GT4";
    if (static_cast<int32_t>(count) <= 0) return "COUNT_NONPOSITIVE";
    return "ELIGIBLE_AT_ENTRY";
}
void adapter(ppu_context* c, void (*original)(ppu_context*), const char* name,
             std::atomic<unsigned>& counter) {
    if (!enabled()) { original(c); return; }
    const unsigned n = ++counter;
    if (candidate_id) { ++candidate_adapters; ++candidate_adapter_count; }
    const uint64_t prior = adapter_caller; adapter_caller = c->lr;
    if (n <= 3) log("ADAPTER_ENTER adapter=%s n=%u candidate=%u caller_lr=%08llX r3=%08llX width=%lld count=%lld destination=%08llX flag=%llu",
        name, n, candidate_id, static_cast<unsigned long long>(c->lr),
        static_cast<unsigned long long>(c->gpr[3]), static_cast<long long>(c->gpr[4]),
        static_cast<long long>(c->gpr[5]), static_cast<unsigned long long>(c->gpr[6]),
        static_cast<unsigned long long>(name[7] == 'C' ? c->gpr[7] & 255 : 0));
    original(c);
    if (n <= 3) log("ADAPTER_RETURN adapter=%s n=%u candidate=%u", name, n, candidate_id);
    adapter_caller = prior;
}
struct Monitor {
    Monitor() {
        if (!enabled()) return;
        log("ARM window_seconds=35 request=0006E988 candidate=009DDA00 old_probes=OFF");
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::seconds(35));
            const Pointers p = pointers();
            log("WINDOW_35S candidate_entries=%u candidate_adapter_calls=%u adapter_ac=%u adapter_f0=%u request_entries=%u builder_calls=%u append_calls=%u append_write_calls=%u outer_ok=%d outer=%08X control_ok=%d control=%08X",
                candidates.load(), candidate_adapters.load(), adapter_ac.load(), adapter_f0.load(),
                requests.load(), builders.load(), appends.load(), writes.load(),
                p.outer_ok, p.outer, p.control_ok, p.control);
        }).detach();
    }
} monitor;
}

extern "C" void __wrap__Z13func_009DDA00P11ppu_context(ppu_context* c) {
    if (!enabled()) { __real__Z13func_009DDA00P11ppu_context(c); return; }
    const unsigned n = ++candidates, prior = candidate_id; candidate_id = n;
    const unsigned prior_count = candidate_adapter_count; candidate_adapter_count = 0;
    const int32_t mode = static_cast<int32_t>(c->gpr[4]);
    const unsigned flag = static_cast<uint32_t>(c->gpr[6]) & 255;
    const char* exclusion = mode == 11 ? "MODE_11" : !flag ? "FLAG_ZERO" : !mode ? "MODE_ZERO" : "NONE_AT_ENTRY";
    if (n <= 3) log("CANDIDATE_ENTER n=%u caller_lr=%08llX object=%08llX mode=%d arg5=%llu flag=%u entry_exclusion=%s",
        n, static_cast<unsigned long long>(c->lr), static_cast<unsigned long long>(c->gpr[3]),
        mode, static_cast<unsigned long long>(c->gpr[5]), flag, exclusion);
    __real__Z13func_009DDA00P11ppu_context(c);
    if (n <= 3) log("CANDIDATE_RETURN n=%u adapters_during_candidate=%u", n, candidate_adapter_count);
    candidate_id = prior; candidate_adapter_count = prior_count;
}
extern "C" void __wrap__Z13func_0006EBACP11ppu_context(ppu_context* c) {
    adapter(c, __real__Z13func_0006EBACP11ppu_context, "0006EBAC", adapter_ac);
}
extern "C" void __wrap__Z13func_0006EBF0P11ppu_context(ppu_context* c) {
    adapter(c, __real__Z13func_0006EBF0P11ppu_context, "0006EBF0", adapter_f0);
}
extern "C" void __wrap__Z13func_0006E988P11ppu_context(ppu_context* c) {
    if (!enabled()) { __real__Z13func_0006E988P11ppu_context(c); return; }
    Request request{++requests, 0, 0, 0, adapter_caller, false};
    request.emit = request.id <= 3;
    Request* prior = active; active = &request;
    const Pointers p = pointers();
    if (request.emit) log("REQUEST_ENTER id=%u candidate=%u caller_lr=%08llX adapter_caller=%08llX outer_ok=%d outer=%08X control_ok=%d control=%08X source=%08llX source_format=%08llX width=%lld count=%lld destination=%08llX destination_format=%08llX flag=%llu entry_predicate=%s",
        request.id, candidate_id, static_cast<unsigned long long>(c->lr),
        static_cast<unsigned long long>(request.adapter_caller), p.outer_ok, p.outer, p.control_ok, p.control,
        static_cast<unsigned long long>(c->gpr[3]), static_cast<unsigned long long>(c->gpr[4]),
        static_cast<long long>(c->gpr[5]), static_cast<long long>(c->gpr[6]),
        static_cast<unsigned long long>(c->gpr[7]), static_cast<unsigned long long>(c->gpr[8]),
        static_cast<unsigned long long>(c->gpr[9]), entry_predicate(c,p));
    __real__Z13func_0006E988P11ppu_context(c);
    const Pointers after = pointers();
    if (request.emit) log("REQUEST_RETURN id=%u builders=%u appends=%u append_writes=%u outer_after=%08X control_after=%08X cr=%08X",
        request.id, request.built, request.appended, request.written, after.outer, after.control, c->cr);
    active = prior;
}
extern "C" void __wrap__Z13func_00A55540P11ppu_context(ppu_context* c) {
    const bool watch = enabled() && active;
    unsigned n=0;
    if (watch) {
        ++builders; n=++active->built;
        if (active->emit && n<=4) log("BUILDER_ENTER id=%u batch=%u caller_lr=%08llX source=%08llX source_format=%08llX width=%lld batch_count=%llu destination=%08llX destination_format=%08llX flag=%llu",
            active->id,n,static_cast<unsigned long long>(c->lr),static_cast<unsigned long long>(c->gpr[4]),
            static_cast<unsigned long long>(c->gpr[5]),static_cast<long long>(c->gpr[6]),
            static_cast<unsigned long long>(c->gpr[7]),static_cast<unsigned long long>(c->gpr[8]),
            static_cast<unsigned long long>(c->gpr[9]),static_cast<unsigned long long>(c->gpr[10]));
    }
    __real__Z13func_00A55540P11ppu_context(c);
    if (watch && active->emit && n<=4) log("BUILDER_RETURN id=%u batch=%u",active->id,n);
}
extern "C" void __wrap__Z13func_009FA818P11ppu_context(ppu_context* c) {
    const bool watch=enabled() && active;
    unsigned n=0;
    if (watch) {
        ++appends; n=++active->appended;
        if (active->emit && n<=4) log("APPEND_ENTER id=%u batch=%u caller_lr=%08llX control=%08llX marker=%08llX",
            active->id,n,static_cast<unsigned long long>(c->lr),static_cast<unsigned long long>(c->gpr[4]),
            static_cast<unsigned long long>(c->gpr[3]));
    }
    __real__Z13func_009FA818P11ppu_context(c);
    if (watch && active->emit && n<=4) log("APPEND_RETURN id=%u batch=%u",active->id,n);
}
extern "C" void __wrap__Z13func_009FA42CP11ppu_context(ppu_context* c) {
    const bool watch=enabled() && active;
    unsigned n=0;
    if (watch) {
        ++writes; n=++active->written;
        if (active->emit && n<=4) log("APPEND_WRITE_ENTER id=%u batch=%u caller_lr=%08llX descriptor_control=%08llX",
            active->id,n,static_cast<unsigned long long>(c->lr),static_cast<unsigned long long>(c->gpr[3]));
    }
    __real__Z13func_009FA42CP11ppu_context(c);
    if (watch && active->emit && n<=4) log("APPEND_WRITE_RETURN id=%u batch=%u returned_next_index=%llu",
        active->id,n,static_cast<unsigned long long>(c->gpr[3]));
}
