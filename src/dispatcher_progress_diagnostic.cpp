/* TASK-024: read-only, bounded dispatcher/caller observation. Default OFF.
 * Each original is forwarded once; no register/memory/return modification.
 * State edges are restricted to 000136F8's own frame, never descendants.
 */
#include "ppu_context.h"
#include <windows.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

extern "C" uint8_t* vm_base;
extern "C" void __real__Z13func_00011CE8P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_000136F8P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_00014EB4P11ppu_context(ppu_context*);
extern "C" void __real_ps3_indirect_call(ppu_context*);

namespace {
std::atomic<unsigned> startups{0}, startup_returns{0}, entries{0}, returns{0};
std::atomic<unsigned> caller_entries{0}, caller_returns{0}, downstream{0}, downstream_returns{0};
std::atomic<unsigned> edges{0}, edge_returns{0}, last_lr{0}, last_code{0}, last_object{0}, phase{0};
std::atomic<unsigned> manager{0};
thread_local ppu_context* dispatch_context=nullptr;
thread_local uint64_t dispatch_frame=0;
thread_local bool startup_active=false, completed_dispatch=false;
bool enabled() {
    static bool on=[] {const char* p=std::getenv("MLB16_TASK024_TRACE");
        return p && std::strcmp(p,"1")==0;}();
    return on;
}
void log(const char* fmt, ...) {
    const int saved_errno=errno; const DWORD saved_error=GetLastError();
    char line[1400];va_list ap;va_start(ap,fmt);std::vsnprintf(line,sizeof line,fmt,ap);va_end(ap);
    _lock_file(stdout);_lock_file(stderr);
    std::fprintf(stderr,"[task024] %s\n",line);std::fflush(stderr);
    _unlock_file(stderr);_unlock_file(stdout);
    errno=saved_errno;SetLastError(saved_error);
}
uint32_t word(uint32_t ea) {
    const DWORD saved_error=GetLastError();uint32_t raw=0;SIZE_T done=0;
    bool ok=vm_base && ea<=0xFFFFFFFBu &&
        ReadProcessMemory(GetCurrentProcess(),vm_base+ea,&raw,4,&done) && done==4;
    SetLastError(saved_error);return ok?__builtin_bswap32(raw):0;
}
void state(const char* event, uint32_t m) {
    uint32_t object=word(m+0x24),vt=object?word(object):0;
    log("%s manager=%08X manager_vtable=%08X count=%u array=%08X current=%08X vtable=%08X slot08=%08X slot0C=%08X slot10=%08X slot14=%08X",
        event,m,word(m),word(m+0x18),word(m+0x20),object,vt,
        vt?word(word(vt+8)):0,vt?word(word(vt+12)):0,
        vt?word(word(vt+16)):0,vt?word(word(vt+20)):0);
}
void summary(unsigned seconds) {
    log("SUMMARY seconds=%u startup=%u/%u caller=%u/%u dispatch=%u/%u downstream=%u/%u state_edges=%u/%u last_lr=%08X last_code=%08X last_object=%08X phase=%u",
        seconds,startups.load(),startup_returns.load(),caller_entries.load(),caller_returns.load(),
        entries.load(),returns.load(),downstream.load(),downstream_returns.load(),edges.load(),edge_returns.load(),
        last_lr.load(),last_code.load(),last_object.load(),phase.load());
    if(manager.load())state("STATE_SNAPSHOT",manager.load());
}
struct Monitor {
    Monitor() {
        if(!enabled())return;
        log("ARM first_state_edges=8 summaries=35,170 timing_controls=unchanged");
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::seconds(35));summary(35);
            std::this_thread::sleep_for(std::chrono::seconds(135));summary(170);
        }).detach();
    }
} monitor;
}

extern "C" void __wrap__Z13func_00011CE8P11ppu_context(ppu_context* c) {
    if(!enabled()) {__real__Z13func_00011CE8P11ppu_context(c);return;}
    unsigned n=++startups;bool previous=startup_active;startup_active=true;
    if(n<=2)log("STARTUP_ENTER n=%u tid=%llu caller_lr=%08llX object=%08llX",n,
        (unsigned long long)c->thread_id,(unsigned long long)c->lr,(unsigned long long)c->gpr[3]);
    __real__Z13func_00011CE8P11ppu_context(c);
    ++startup_returns;if(n<=2)log("STARTUP_RETURN n=%u",n);
    startup_active=previous;
}
extern "C" void __wrap__Z13func_000136F8P11ppu_context(ppu_context* c) {
    if(!enabled()) {__real__Z13func_000136F8P11ppu_context(c);return;}
    unsigned n=++entries;uint32_t m=uint32_t(c->gpr[3]);manager.store(m);
    auto* previous=dispatch_context;uint64_t old_frame=dispatch_frame;
    dispatch_context=c;dispatch_frame=c->gpr[1]-0xA0;
    if(n<=2) {
        log("DISPATCH_ENTER n=%u tid=%llu caller_lr=%08llX sp=%08llX startup_active=%u",n,
            (unsigned long long)c->thread_id,(unsigned long long)c->lr,
            (unsigned long long)c->gpr[1],unsigned(startup_active));
        state("DISPATCH_STATE",m);
    }
    __real__Z13func_000136F8P11ppu_context(c);
    ++returns;completed_dispatch=true;
    if(n<=2) {log("DISPATCH_RETURN n=%u lr=%08llX",n,(unsigned long long)c->lr);state("RETURN_STATE",m);}
    dispatch_context=previous;dispatch_frame=old_frame;
}
extern "C" void __wrap_ps3_indirect_call(ppu_context* c) {
    if(!enabled()) {__real_ps3_indirect_call(c);return;}
    const uint32_t lr=uint32_t(c->lr),target=uint32_t(c->ctr);
    bool caller=startup_active && lr==0x1353C && target==0x136F8;
    bool selected=c==dispatch_context && c->gpr[1]==dispatch_frame &&
        (lr==0x13750 || lr==0x13778 || lr==0x137A0 || lr==0x137E8 || lr==0x1383C);
    unsigned n=0;
    if(caller && ++caller_entries<=2)log("CALLER_DISPATCH_ENTER caller=00011CE8 lr=%08X target=%08X",lr,target);
    if(selected) {
        n=++edges;last_lr.store(lr);last_code.store(target);last_object.store(uint32_t(c->gpr[3]));phase.store(1);
        if(n<=8)log("STATE_EDGE_ENTER n=%u lr=%08X code=%08X object=%08X arg4=%08X",n,lr,target,
            uint32_t(c->gpr[3]),uint32_t(c->gpr[4]));
    }
    __real_ps3_indirect_call(c);
    if(selected) {++edge_returns;phase.store(2);if(n<=8)log("STATE_EDGE_RETURN n=%u lr=%08X result=%08X",n,lr,uint32_t(c->gpr[3]));}
    if(caller && ++caller_returns<=2)log("CALLER_DISPATCH_RETURN continuation=0001353C");
}
extern "C" void __wrap__Z13func_00014EB4P11ppu_context(ppu_context* c) {
    bool selected=enabled() && startup_active && completed_dispatch && c->lr==0x13544;
    unsigned n=selected?++downstream:0;
    if(n && n<=2)log("POST_DISPATCH_ENTER code=00014EB4 caller_lr=00013544");
    __real__Z13func_00014EB4P11ppu_context(c);
    if(n) {++downstream_returns;if(n<=2)log("POST_DISPATCH_RETURN code=00014EB4");}
}
