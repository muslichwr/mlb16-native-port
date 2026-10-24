/* TASK-023: bounded observation, no guest writes or callback replacement.
 * Requires the default-OFF build option and MLB16_TASK023_TRACE=1.
 * Only main-thread 00066530 -> 00F25BCC at LR 00066BE0 is selected.
 * Every original runs once. Async ticker callbacks are excluded by host TLS.
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
extern "C" void __real__Z13func_00066530P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_00F25BCCP11ppu_context(ppu_context*);
extern "C" void __real__Z13func_00011CE8P11ppu_context(ppu_context*);
extern "C" void __real__Z13func_000136F8P11ppu_context(ppu_context*);
extern "C" uint64_t __real_ppu_guest_call(uint32_t,uint64_t,uint64_t,uint64_t,uint64_t,
                                        uint64_t,uint64_t,uint64_t,uint64_t);

namespace {
enum Phase : unsigned { IMPORT_ENTER=1, CALLBACK_ENTER, CALLBACK_RETURN, IMPORT_RETURN };
constexpr unsigned slots=16;
struct Sequence {
    std::atomic<unsigned> id{0}, path{0}, phase{0}, buffer{0};
    std::atomic<unsigned> cb_entries{0}, cb_returns{0}, opd{0}, code{0};
    std::atomic<uint64_t> result{0};
};
Sequence recent[slots];
std::atomic<unsigned> paths{0}, path_returns{0}, flips{0}, flip_returns{0};
std::atomic<unsigned> callbacks{0}, callback_returns{0}, last_path_return{0}, last_flip_return{0};
std::atomic<unsigned> startups{0}, startup_returns{0}, dispatches{0};
thread_local unsigned current_path=0, current_flip=0;
thread_local bool inside_callback=false;
bool enabled() {
    static bool on=[] {const char* e=std::getenv("MLB16_TASK023_TRACE");
        return e && std::strcmp(e,"1")==0;}();
    return on;
}
void log(const char* fmt, ...) {
    const int saved_errno=errno; const DWORD saved_error=GetLastError();
    char line[1200];va_list ap;va_start(ap,fmt);std::vsnprintf(line,sizeof line,fmt,ap);va_end(ap);
    _lock_file(stdout);_lock_file(stderr);
    std::fprintf(stderr,"[task023] %s\n",line);std::fflush(stderr);
    _unlock_file(stderr);_unlock_file(stdout);
    errno=saved_errno;SetLastError(saved_error);
}
uint32_t opd_code(uint32_t ea) {
    const DWORD saved_error=GetLastError();uint32_t raw=0;SIZE_T done=0;
    bool ok=vm_base && ea<=0xFFFFFFFBu &&
        ReadProcessMemory(GetCurrentProcess(),vm_base+ea,&raw,4,&done) && done==4;
    SetLastError(saved_error);return ok?__builtin_bswap32(raw):0;
}
void summary(unsigned seconds) {
    unsigned count=flips.load();
    log("SUMMARY seconds=%u paths=%u path_returns=%u last_path_return=%u imports=%u import_returns=%u last_flip_return=%u callbacks=%u callback_returns=%u startup_entries=%u startup_returns=%u dispatcher_entries=%u",
        seconds,paths.load(),path_returns.load(),last_path_return.load(),count,flip_returns.load(),
        last_flip_return.load(),callbacks.load(),callback_returns.load(),startups.load(),startup_returns.load(),dispatches.load());
    const unsigned first=count>3?count-3:1;
    for(unsigned i=first;i<=count;++i) {
        Sequence& r=recent[i%slots];if(r.id.load()!=i)continue;
        unsigned path=r.path.load(),phase=r.phase.load(),buffer=r.buffer.load();
        unsigned ce=r.cb_entries.load(),cr=r.cb_returns.load(),opd=r.opd.load(),code=r.code.load();
        uint64_t result=r.result.load();if(r.id.load()!=i)continue;
        log("RECENT seconds=%u seq=%u path=%u phase=%u buffer=%u cb_entries=%u cb_returns=%u opd=%08X code=%08X result=%llu",
            seconds,i,path,phase,buffer,ce,cr,opd,code,(unsigned long long)result);
    }
}
struct Monitor {
    Monitor() {
        if(!enabled())return;
        log("ARM first_sequences=3 recent_slots=16 summaries=35,170 timing_controls=unchanged");
        std::thread([] {
            std::this_thread::sleep_for(std::chrono::seconds(35));summary(35);
            std::this_thread::sleep_for(std::chrono::seconds(135));summary(170);
        }).detach();
    }
} monitor;
}

extern "C" void __wrap__Z13func_00066530P11ppu_context(ppu_context* c) {
    if(!enabled() || c->thread_id!=1 || current_path) {__real__Z13func_00066530P11ppu_context(c);return;}
    const unsigned n=++paths,previous=current_path;current_path=n;
    if(n<=3)log("PATH_ENTER path=%u caller_lr=%08llX",n,(unsigned long long)c->lr);
    __real__Z13func_00066530P11ppu_context(c);
    ++path_returns;last_path_return.store(n);
    if(n<=3)log("PATH_RETURN path=%u",n);
    current_path=previous;
}

extern "C" void __wrap__Z13func_00F25BCCP11ppu_context(ppu_context* c) {
    if(!enabled() || !current_path || current_flip || c->thread_id!=1 || c->lr!=0x66BE0) {
        __real__Z13func_00F25BCCP11ppu_context(c);return;
    }
    const unsigned n=++flips,previous=current_flip;current_flip=n;
    Sequence& r=recent[n%slots];r.id.store(0);r.path.store(current_path);r.buffer.store(unsigned(c->gpr[4]));
    r.cb_entries.store(0);r.cb_returns.store(0);r.opd.store(0);r.code.store(0);r.result.store(0);
    r.phase.store(IMPORT_ENTER);r.id.store(n);
    if(n<=3)log("IMPORT_ENTER seq=%u path=%u caller_lr=00066BE0 buffer=%u",n,current_path,r.buffer.load());
    __real__Z13func_00F25BCCP11ppu_context(c);
    r.result.store(c->gpr[3]);r.phase.store(IMPORT_RETURN);++flip_returns;last_flip_return.store(n);
    if(n<=3)log("IMPORT_RETURN seq=%u result=%llu",n,(unsigned long long)c->gpr[3]);
    current_flip=previous;
}

extern "C" uint64_t __wrap_ppu_guest_call(uint32_t opd,uint64_t a0,uint64_t a1,uint64_t a2,uint64_t a3,
                                          uint64_t a4,uint64_t a5,uint64_t a6,uint64_t a7) {
    const unsigned n=current_flip;
    if(!n || inside_callback)return __real_ppu_guest_call(opd,a0,a1,a2,a3,a4,a5,a6,a7);
    inside_callback=true;
    Sequence& r=recent[n%slots];r.opd.store(opd);r.code.store(opd_code(opd));
    ++callbacks;++r.cb_entries;r.phase.store(CALLBACK_ENTER);
    if(n<=3)log("CALLBACK_ENTER seq=%u opd=%08X code=%08X arg0=%llu",n,opd,r.code.load(),(unsigned long long)a0);
    const uint64_t result=__real_ppu_guest_call(opd,a0,a1,a2,a3,a4,a5,a6,a7);
    ++callback_returns;++r.cb_returns;r.phase.store(CALLBACK_RETURN);
    if(n<=3)log("CALLBACK_RETURN seq=%u result=%llu",n,(unsigned long long)result);
    inside_callback=false;
    return result;
}

extern "C" void __wrap__Z13func_00011CE8P11ppu_context(ppu_context* c) {
    if(!enabled()) {__real__Z13func_00011CE8P11ppu_context(c);return;}
    unsigned n=++startups;if(n<=2)log("STARTUP_ENTER n=%u",n);
    __real__Z13func_00011CE8P11ppu_context(c);
    ++startup_returns;if(n<=2)log("STARTUP_RETURN n=%u",n);
}
extern "C" void __wrap__Z13func_000136F8P11ppu_context(ppu_context* c) {
    if(enabled() && ++dispatches<=2)log("DISPATCHER_ENTER");
    __real__Z13func_000136F8P11ppu_context(c);
}
