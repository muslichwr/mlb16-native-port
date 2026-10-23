/* TASK-020: original, bounded observation of startup -> state dispatch.
 * Requires the default-OFF build option and MLB16_TASK020_TRACE=1.
 * Forwards each original once; never writes guest memory or context.
 * The single live snapshot is asynchronous evidence, not an atomic state dump.
 */
#include "ppu_context.h"
#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <thread>

extern "C" uint8_t* vm_base;
extern "C" void __real__Z13func_00011CE8P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_000136F8P11ppu_context(ppu_context*);

namespace {
std::atomic<unsigned> startups{0}, startup_returns{0}, dispatches{0}, dispatch_returns{0};
std::atomic<ppu_context*> live_startup{nullptr};
std::atomic<uint32_t> manager{0};
bool enabled() {
    static const bool on = [] { const char* e = std::getenv("MLB16_TASK020_TRACE");
        return e && std::strcmp(e, "1") == 0; }();
    return on;
}
void log(const char* fmt, ...) {
    char line[1536]; va_list ap; va_start(ap, fmt);
    std::vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    _lock_file(stdout); _lock_file(stderr);
    std::fprintf(stderr, "[task020] %s\n", line); std::fflush(stderr);
    _unlock_file(stderr); _unlock_file(stdout);
}
bool read32(uint32_t ea, uint32_t& value) {
    if (!vm_base || (ea & 3)) return false;
    uint32_t raw = 0; SIZE_T done = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), vm_base + ea, &raw, 4, &done) || done != 4) return false;
    value = __builtin_bswap32(raw); return true;
}
bool read64(uint32_t ea, uint64_t& value) {
    uint32_t hi=0, lo=0;
    if (ea > 0xFFFFFFF7u || !read32(ea, hi) || !read32(ea+4, lo)) return false;
    value = (uint64_t(hi)<<32) | lo; return true;
}
void state(const char* when, uint32_t obj) {
    uint32_t current=0, vt=0, opd=0, code=0;
    bool ok = obj && read32(obj+0x24,current);
    bool target_ok = ok && current && read32(current,vt) &&
        read32(vt+0xC,opd) && read32(opd,code);
    log("STATE when=%s manager=%08X current_ok=%d current=%08X target_ok=%d update_code=%08X",
        when,obj,ok,current,target_ok,code);
    for (uint32_t ea : {0x0114E8ECu,0x01346088u,0x0113E618u,0x01346070u,0x012D8F38u}) {
        uint32_t v=0; bool valid=read32(ea,v);
        log("GLOBAL when=%s ea=%08X ok=%d word=%08X",when,ea,valid,v);
    }
}
struct Monitor {
    Monitor() {
        if (!enabled()) return;
        log("ARM window_seconds=35 boundary=00011CE8_to_000136F8 old_probes=OFF");
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::seconds(35));
            log("WINDOW_35S startup_entries=%u startup_returns=%u dispatch_entries=%u dispatch_returns=%u",
                startups.load(),startup_returns.load(),dispatches.load(),dispatch_returns.load());
            state("35s",manager.load());
            ppu_context snap{}; SIZE_T done=0; ppu_context* c=live_startup.load();
            if (!c || !ReadProcessMemory(GetCurrentProcess(),c,&snap,sizeof snap,&done) || done!=sizeof snap) {
                log("LIVE_CONTEXT unavailable=1"); return;
            }
            log("LIVE_CONTEXT tid=%llu cia=%08llX lr=%08llX sp=%08llX r3=%08llX r4=%08llX r13=%08llX r27=%08llX r28=%08llX r29=%08llX r30=%08llX r31=%08llX",
                (unsigned long long)snap.thread_id,(unsigned long long)snap.cia,(unsigned long long)snap.lr,
                (unsigned long long)snap.gpr[1],(unsigned long long)snap.gpr[3],(unsigned long long)snap.gpr[4],
                (unsigned long long)snap.gpr[13],(unsigned long long)snap.gpr[27],(unsigned long long)snap.gpr[28],
                (unsigned long long)snap.gpr[29],(unsigned long long)snap.gpr[30],(unsigned long long)snap.gpr[31]);
            uint32_t sp=uint32_t(snap.gpr[1]);
            for (unsigned i=0;i<12;++i) {
                uint64_t parent=0, saved=0;
                if ((sp&15) || sp>0xFFFFFFE0u || !read64(sp,parent) || !read64(sp+0x10,saved)) break;
                log("STACK depth=%u sp=%08X parent=%08llX saved_lr=%08llX",i,sp,
                    (unsigned long long)parent,(unsigned long long)saved);
                if (parent<=sp || parent>uint64_t(sp)+0x100000 || parent>0xFFFFFFFFu) break;
                sp=uint32_t(parent);
            }
        }).detach();
    }
} monitor;
}

extern "C" void __wrap__Z13func_00011CE8P11ppu_context(ppu_context* c) {
    if (!enabled()) { __real__Z13func_00011CE8P11ppu_context(c); return; }
    unsigned n=++startups; manager.store(uint32_t(c->gpr[3])); live_startup.store(c);
    if (n<=2) log("STARTUP_ENTER n=%u caller_lr=%08llX manager=%08X",n,(unsigned long long)c->lr,manager.load());
    __real__Z13func_00011CE8P11ppu_context(c);
    live_startup.store(nullptr); ++startup_returns;
    if (n<=2) log("STARTUP_RETURN n=%u",n);
}
extern "C" void __wrap__Z13func_000136F8P11ppu_context(ppu_context* c) {
    if (!enabled()) { __real__Z13func_000136F8P11ppu_context(c); return; }
    unsigned n=++dispatches;
    if (n<=2) { log("DISPATCH_ENTER n=%u caller_lr=%08llX",n,(unsigned long long)c->lr); state("dispatch",uint32_t(c->gpr[3])); }
    __real__Z13func_000136F8P11ppu_context(c);
    ++dispatch_returns; if (n<=2) log("DISPATCH_RETURN n=%u",n);
}
