/* TASK-021B: a bounded, default-OFF observation of one guest validation.
 * Requires MLB16_TASK021B_DIAGNOSTIC and MLB16_TASK021B_TRACE=1.
 * Originals run exactly once. No guest memory/context, return, or flag writes.
 * Run only with external protection of the game files (see boot harness).
 * TASK-022 mode reuses this observer and adds only startup/dispatch counters.
 */
#include "ppu_context.h"
#include <windows.h>
#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#ifdef MLB16_TASK022_DIAGNOSTIC
#include <chrono>
#include <thread>
extern "C" void __real__Z13func_00011CE8P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_000136F8P11ppu_context(ppu_context*);
#ifdef MLB16_TASK022R_DIAGNOSTIC
extern "C" void __real__Z13func_000765E4P11ppu_context(ppu_context*);
#endif
#endif

extern "C" uint8_t* vm_base;
extern "C" void __real__Z13func_00016D54P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_0007C174P11ppu_context(ppu_context*);
#ifndef MLB16_TASK022_DIAGNOSTIC
extern "C" void __real__Z13func_00076BACP11ppu_context(ppu_context*);
#endif
extern "C" void __real__Z13func_00F2610CP11ppu_context(ppu_context*);
extern "C" int __real_open(const char*, int, ...);

namespace {
std::atomic<unsigned> entries{0}, dialogs{0};
#ifdef MLB16_TASK022_DIAGNOSTIC
#ifdef MLB16_TASK022R_DIAGNOSTIC
constexpr const char* trace_env = "MLB16_TASK022R_TRACE";
constexpr const char* trace_tag = "task022r";
std::atomic<ppu_context*> main_context{nullptr}, utility_context{nullptr};
std::atomic<unsigned> cache_opens{0}, cache_writes{0};
#else
constexpr const char* trace_env = "MLB16_TASK022_TRACE";
constexpr const char* trace_tag = "task022";
#endif
std::atomic<unsigned> startup_entries{0}, startup_returns{0}, dispatch_entries{0};
#else
constexpr const char* trace_env = "MLB16_TASK021B_TRACE";
constexpr const char* trace_tag = "task021b";
#endif
struct Observation {
    unsigned id = 0, opens = 0;
    bool active = false, returned = false;
    uint64_t disc = 0, hdd = 0, result = 0;
    bool have_disc = false, have_hdd = false;
};
thread_local Observation observation;
bool enabled() {
    static const bool on = [] { const char* e = std::getenv(trace_env);
        return e && std::strcmp(e, "1") == 0; }();
    return on;
}
void log(const char* fmt, ...) {
    const int saved_errno = errno; const DWORD saved_error = GetLastError();
    char line[2048]; va_list ap; va_start(ap, fmt);
    std::vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    _lock_file(stdout); _lock_file(stderr);
    std::fprintf(stderr, "[%s] %s\n", trace_tag, line); std::fflush(stderr);
    _unlock_file(stderr); _unlock_file(stdout);
    errno = saved_errno; SetLastError(saved_error);
}
bool read_guest(uint32_t ea, void* dest, size_t length) {
    if (!vm_base || uint64_t(ea) + length > 0x100000000ULL) return false;
    const int saved_errno = errno; const DWORD saved_error = GetLastError();
    SIZE_T done = 0;
    bool ok = ReadProcessMemory(GetCurrentProcess(), vm_base + ea, dest, length, &done) && done == length;
    errno = saved_errno; SetLastError(saved_error); return ok;
}
void text(uint32_t ea, char* dest, size_t length) {
    size_t i = 0;
    for (; i + 1 < length; ++i) {
        char ch = 0;
        if (!read_guest(ea + uint32_t(i), &ch, 1) || !ch) break;
        dest[i] = ch == '\n' || ch == '\r' ? ' ' : ch;
    }
    dest[i] = 0;
}
}

extern "C" void __wrap__Z13func_00016D54P11ppu_context(ppu_context* c) {
    if (!enabled()) { __real__Z13func_00016D54P11ppu_context(c); return; }
    unsigned n = ++entries;
    if (n > 2) { __real__Z13func_00016D54P11ppu_context(c); return; }
    observation = {}; observation.id = n; observation.active = true;
#ifdef MLB16_TASK022R_DIAGNOSTIC
    utility_context.store(c);
#endif
    const auto caller = c->lr;
    log("ENTER id=%u tid=%llu caller_lr=%08llX vfs_root=%s hdd0_root=%s", n,
        (unsigned long long)c->thread_id, (unsigned long long)caller,
        std::getenv("PS3_VFS_ROOT") ? std::getenv("PS3_VFS_ROOT") : "UNSET",
        std::getenv("PS3_HDD0_ROOT") ? std::getenv("PS3_HDD0_ROOT") : "UNSET");
    __real__Z13func_00016D54P11ppu_context(c);
    observation.active = false; observation.returned = true; observation.result = c->gpr[3];
    log("RETURN id=%u tid=%llu caller_lr=%08llX value=%llu disc_seen=%d hdd_seen=%d disc=%llu hdd=%llu", n,
        (unsigned long long)c->thread_id, (unsigned long long)caller,
        (unsigned long long)observation.result, observation.have_disc, observation.have_hdd,
        (unsigned long long)observation.disc, (unsigned long long)observation.hdd);
}

extern "C" void __wrap__Z13func_0007C174P11ppu_context(ppu_context* c) {
    const auto caller = c->lr;
    const bool selected = observation.active && (caller == 0x16FD8 || caller == 0x16FF0);
    const auto device = c->gpr[5];
#ifndef MLB16_TASK022_DIAGNOSTIC
    const auto saved_disc = c->gpr[25];
#endif
    char filename[96]{};
    if (selected) text(uint32_t(c->gpr[4]), filename, sizeof filename);
    __real__Z13func_0007C174P11ppu_context(c);
    if (!selected) return;
    if (caller == 0x16FD8) {
        observation.disc = c->gpr[3]; observation.have_disc = true;
    } else { observation.hdd = c->gpr[3]; observation.have_hdd = true; }
#ifdef MLB16_TASK022_DIAGNOSTIC
    log("LENGTH id=%u tid=%llu caller_lr=%08llX file=%s device=%llu value=%llu",
        observation.id, (unsigned long long)c->thread_id, (unsigned long long)caller, filename,
        (unsigned long long)device, (unsigned long long)c->gpr[3]);
#else
    log("LENGTH id=%u tid=%llu caller_lr=%08llX file=%s device=%llu value=%llu saved_r25=%llu eligibility_r27=%llu equal_to_r25=%d",
        observation.id, (unsigned long long)c->thread_id, (unsigned long long)caller, filename,
        (unsigned long long)device, (unsigned long long)c->gpr[3], (unsigned long long)saved_disc,
        (unsigned long long)c->gpr[27], c->gpr[3] == saved_disc);
#endif
}

#ifndef MLB16_TASK022_DIAGNOSTIC
extern "C" void __wrap__Z13func_00076BACP11ppu_context(ppu_context* c) {
    if (observation.active && c->lr == 0x1702C)
        log("MISMATCH_BRANCH id=%u tid=%llu caller_lr=0001702C eligibility_r27=%llu cr=%08X disc=%llu hdd=%llu",
            observation.id, (unsigned long long)c->thread_id, (unsigned long long)c->gpr[27], unsigned(c->cr),
            (unsigned long long)observation.disc, (unsigned long long)observation.hdd);
    __real__Z13func_00076BACP11ppu_context(c);
}
#endif

extern "C" void __wrap__Z13func_00F2610CP11ppu_context(ppu_context* c) {
    if (enabled() && c->lr == 0x18FC8 && ++dialogs <= 2) {
#ifdef MLB16_TASK022_DIAGNOSTIC
        log("ERROR_PATH id=%u tid=%llu caller_lr=00018FC8 validator_return_seen=%d validator_result=%llu",
            observation.id, (unsigned long long)c->thread_id, observation.returned,
            (unsigned long long)observation.result);
#else
        char message[640]{}; text(uint32_t(c->gpr[4]), message, sizeof message);
        unsigned char flag = 0; bool ok = read_guest(0x011B7A02, &flag, 1);
        log("ERROR_PATH id=%u tid=%llu caller_lr=00018FC8 validator_return_seen=%d validator_result=%llu flag_ok=%d flag=%u message=%s",
            observation.id, (unsigned long long)c->thread_id, observation.returned,
            (unsigned long long)observation.result, ok, unsigned(flag), message);
#endif
    }
    __real__Z13func_00F2610CP11ppu_context(c);
}

extern "C" int __wrap_open(const char* path, int flags, ...) {
    int mode = 0;
    if (flags & O_CREAT) { va_list ap; va_start(ap, flags); mode = va_arg(ap, int); va_end(ap); }
    const int result = __real_open(path, flags, mode);
#ifdef MLB16_TASK022R_DIAGNOSTIC
    if (enabled() && path && std::strstr(path, "MLBPS3_SYS.MD5") && ++cache_opens <= 4)
        log("CACHE_OPEN path=%s flags=%X result=%d errno=%d", path, flags, result, errno);
#endif
    if (observation.active && path && std::strstr(path, "MLBPS3_GD.WAD")
#ifdef MLB16_TASK022_DIAGNOSTIC
        && !std::strstr(path, ".HDR")
#endif
        && ++observation.opens <= 8)
        log("HOST_OPEN id=%u path=%s flags=%X result=%d errno=%d", observation.id, path, flags, result, errno);
    return result;
}

#ifdef MLB16_TASK022_DIAGNOSTIC
namespace {
#ifdef MLB16_TASK022R_DIAGNOSTIC
void snapshot(const char* name, ppu_context* c) {
    ppu_context snap{}; SIZE_T done = 0;
    if (!c || !ReadProcessMemory(GetCurrentProcess(),c,&snap,sizeof snap,&done) || done!=sizeof snap) {
        log("LIVE name=%s unavailable=1",name); return;
    }
    log("LIVE name=%s tid=%llu cia=%08llX lr=%08llX sp=%08llX r3=%08llX r4=%08llX r7=%08llX",
        name,(unsigned long long)snap.thread_id,(unsigned long long)snap.cia,(unsigned long long)snap.lr,
        (unsigned long long)snap.gpr[1],(unsigned long long)snap.gpr[3],
        (unsigned long long)snap.gpr[4],(unsigned long long)snap.gpr[7]);
    uint32_t sp=uint32_t(snap.gpr[1]);
    for (unsigned i=0;i<8;++i) {
        uint64_t parent=0,lr=0;
        if ((sp&15) || sp>0xFFFFFFE0u || !read_guest(sp,&parent,8) || !read_guest(sp+16,&lr,8))break;
        parent=__builtin_bswap64(parent);lr=__builtin_bswap64(lr);
        log("STACK name=%s depth=%u sp=%08X parent=%08llX saved_lr=%08llX",name,i,sp,
            (unsigned long long)parent,(unsigned long long)lr);
        if (parent<=sp || parent>uint64_t(sp)+0x100000 || parent>0xFFFFFFFFu)break;
        sp=uint32_t(parent);
    }
}
#endif
struct ProgressSummary {
    ProgressSummary() {
        if (!enabled()) return;
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::seconds(35));
            log("WINDOW_35S validations=%u corrupt_dialog_calls=%u startup_entries=%u startup_returns=%u dispatch_entries=%u",
                entries.load(), dialogs.load(), startup_entries.load(), startup_returns.load(), dispatch_entries.load());
#ifdef MLB16_TASK022R_DIAGNOSTIC
            std::this_thread::sleep_for(std::chrono::seconds(135));
            log("WINDOW_170S validations=%u corrupt_dialog_calls=%u startup_entries=%u startup_returns=%u dispatch_entries=%u cache_opens=%u cache_writes=%u",
                entries.load(),dialogs.load(),startup_entries.load(),startup_returns.load(),dispatch_entries.load(),cache_opens.load(),cache_writes.load());
            snapshot("main",main_context.load()); snapshot("utility",utility_context.load());
#endif
        }).detach();
    }
} progress_summary;
}
extern "C" void __wrap__Z13func_00011CE8P11ppu_context(ppu_context* c) {
    if (!enabled()) { __real__Z13func_00011CE8P11ppu_context(c); return; }
    const unsigned n = ++startup_entries;
#ifdef MLB16_TASK022R_DIAGNOSTIC
    main_context.store(c);
#endif
    const auto caller = c->lr;
    if (n <= 2) log("STARTUP_ENTER n=%u tid=%llu caller_lr=%08llX", n,
        (unsigned long long)c->thread_id, (unsigned long long)caller);
    __real__Z13func_00011CE8P11ppu_context(c);
    ++startup_returns;
    if (n <= 2) log("STARTUP_RETURN n=%u tid=%llu caller_lr=%08llX", n,
        (unsigned long long)c->thread_id, (unsigned long long)caller);
}
extern "C" void __wrap__Z13func_000136F8P11ppu_context(ppu_context* c) {
    if (enabled() && ++dispatch_entries <= 2)
        log("DISPATCH_ENTER tid=%llu caller_lr=%08llX", (unsigned long long)c->thread_id,
            (unsigned long long)c->lr);
    __real__Z13func_000136F8P11ppu_context(c);
}
#endif

#ifdef MLB16_TASK022R_DIAGNOSTIC
extern "C" void __wrap__Z13func_000765E4P11ppu_context(ppu_context* c) {
    const bool selected=enabled() && c->lr==0x17530 && ++cache_writes<=2;
    if (selected)log("CACHE_WRITE_ENTER tid=%llu caller_lr=00017530 file_object=%08llX size=%llu count=%llu",
        (unsigned long long)c->thread_id,(unsigned long long)c->gpr[7],
        (unsigned long long)c->gpr[5],(unsigned long long)c->gpr[6]);
    __real__Z13func_000765E4P11ppu_context(c);
    if (selected)log("CACHE_WRITE_RETURN tid=%llu result=%llu",(unsigned long long)c->thread_id,
        (unsigned long long)c->gpr[3]);
}
#endif
