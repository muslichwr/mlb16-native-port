/* Original read-only differential probe. Standalone; never linked into the game.
 * Exercises the pinned PPU filesystem implementation on existing raw WAD/HDR
 * files. It does not execute the lifted validator or emulate RPCS3.
 * The two dependency adapters below accept only existing, non-NPD inputs whose
 * fallback alternatives do not exist. They abort outside that audited scope.
 */
#include "../third_party/ps3recomp/runtime/ppu/ppu_fs.cpp"
#include <initializer_list>

static void unexpected(const char* what) {
    fprintf(stderr, "REPLAY_ABORT %s\n", what); abort();
}
extern "C" {
uint8_t* vm_base = nullptr;
uint32_t ppu_vm_size = 0;
void vm_write32(uint64_t a, uint32_t v) {
    if (a+4>ppu_vm_size) unexpected("VM write32 bounds");
    v=__builtin_bswap32(v); memcpy(vm_base+a,&v,4);
}
void vm_write64(uint64_t a, uint64_t v) {
    if (a+8>ppu_vm_size) unexpected("VM write64 bounds");
    v=__builtin_bswap64(v); memcpy(vm_base+a,&v,8);
}
void ppu_guest_caller(char*, size_t) { unexpected("caller tracing"); }
void ydkj_host_bt(const char*) { unexpected("backtrace tracing"); }
void ps3_hle_register_ctx(uint32_t, const char*, void (*)(ppu_context*)) {
    unexpected("registration");
}
void ps3_vfs_ps3game_fallback(char* path, size_t) {
    // Native helper leaves these existing paths unchanged. Its 32-bit stat
    // can fail on the >4 GiB archive; reject an available fallback alternative.
    struct _stat64 st{};
    if (_stat64(path,&st)) unexpected("nonexistent host path");
    const char* p=strstr(path,"/PS3_GAME/");
    if (p) {
        char alternate[1200]; snprintf(alternate,sizeof alternate,"%.*s/%s",int(p-path),path,p+10);
        if (_stat64(alternate,&st)==0) unexpected("ambiguous flat fallback");
    } else if ((p=strstr(path,"/USRDIR/"))) {
        char alternate[1200]; snprintf(alternate,sizeof alternate,"%.*s/PS3_GAME%s",int(p-path),path,p);
        if (_stat64(alternate,&st)==0) unexpected("ambiguous nested fallback");
    }
}
}
ps3_guest_caller_fn g_ps3_guest_caller = nullptr;
const char* edat_resolve(const char* path, char*, size_t) {
    FILE* f=fopen(path,"rb"); if (!f) unexpected("EDAT input open");
    unsigned char magic[4]={}; size_t n=fread(magic,1,4,f); fclose(f);
    if (n!=4 || memcmp(magic,"NPD\0",4)==0) unexpected("non-raw input");
    return path; // Same branch as edat_resolve for a verified non-NPD file.
}
static uint32_t rd32(uint32_t a) { uint32_t v; memcpy(&v,vm_base+a,4); return __builtin_bswap32(v); }
static uint64_t rd64(uint32_t a) { uint64_t v; memcpy(&v,vm_base+a,8); return __builtin_bswap64(v); }
static ppu_context args(uint64_t a=0,uint64_t b=0,uint64_t c=0,uint64_t d=0) {
    ppu_context ctx{}; ctx.gpr[3]=a;ctx.gpr[4]=b;ctx.gpr[5]=c;ctx.gpr[6]=d;return ctx;
}
static int probe(const char* guest) {
    strcpy((char*)vm_base+0x100,guest);
    char host[1100]; host_path(host,sizeof host,guest);
    printf("REPLAY path guest=%s host=%s\n",guest,host);
    auto c=args(0x100,0x400); cellFsStat(&c);
    printf("REPLAY stat rc=%08X size=%llu stat_hex=",(uint32_t)c.gpr[3],(unsigned long long)rd64(0x424));
    for(unsigned i=0;i<52;++i)printf("%02X",vm_base[0x400+i]);puts("");
    if(c.gpr[3])return 1;
    c=args(0x100,0,0x500);cellFsOpen(&c); // O_RDONLY only.
    uint32_t fd=rd32(0x500);printf("REPLAY open flags=0 rc=%08X fd=%u\n",(uint32_t)c.gpr[3],fd);
    if(c.gpr[3])return 1;
    c=args(fd,0x400);cellFsFstat(&c);
    printf("REPLAY fstat rc=%08X size=%llu\n",(uint32_t)c.gpr[3],(unsigned long long)rd64(0x424));
    bool hdr=strstr(guest,".HDR")!=nullptr;
    const uint64_t offsets[]={0,463339520ULL,4294967296ULL};
    for(unsigned i=0;i<(hdr?1u:3u);++i) {
        c=args(fd,offsets[i],0,0x510);cellFsLseek(&c);
        printf("REPLAY seek requested=%llu whence=SET rc=%08X actual=%llu\n",(unsigned long long)offsets[i],(uint32_t)c.gpr[3],(unsigned long long)rd64(0x510));
        memset(vm_base+0x600,0xCD,16);c=args(fd,0x600,16,0x520);cellFsRead(&c);
        uint64_t n=rd64(0x520);printf("REPLAY read requested=16 rc=%08X actual=%llu bytes=",(uint32_t)c.gpr[3],(unsigned long long)n);
        if(n>16)unexpected("read count");for(unsigned j=0;j<n;++j)printf("%02X",vm_base[0x600+j]);puts("");
    }
    c=args(fd);cellFsClose(&c);printf("REPLAY close rc=%08X\n",(uint32_t)c.gpr[3]);
    return c.gpr[3]?1:0;
}
int main(int argc,char** argv) {
    if(argc!=2 && argc!=3)return 2;
    static uint8_t memory[65536]={};vm_base=memory;ppu_vm_size=sizeof memory;ppu_vfs_root=argv[1];
    setvbuf(stdout,nullptr,_IONBF,0);
    if(argc==3) {
        if(strcmp(argv[2],"--map-only"))return 2;
        for(const char* guest:{"/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD",
                              "/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD",
                              "/dev_hdd1/cache/BCUS01089/MLBPS3_SYS.MD5"}) {
            char host[1100];host_path(host,sizeof host,guest);
            printf("MAP|%s|%s\n",guest,host);
        }
        return 0;
    }
    int errors=0;
    for(const char* path:{"/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD.HDR",
                         "/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD.HDR",
                         "/dev_bdvd/PS3_GAME/USRDIR/MLBPS3_GD.WAD",
                         "/dev_hdd0/game/BCUS01089_R/USRDIR/MLBPS3_GD.WAD"})errors+=probe(path);
    return errors?1:0;
}
