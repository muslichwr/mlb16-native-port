"""Hold read-only Windows sharing locks during one validation comparison boot.

Existing files cannot be opened for writing or deletion while held. No ACL,
attribute, content, or game mount is changed. Inventory/hash verification is
performed while the locks remain held. --check-only does not launch the game.
Local artifacts belong exclusively in the canonical build, never in Git.
--task022 uses the intact RPCS3 HDD root and a separate one-boot guard.

Limitation observed in TASK-022: sharing locks do not prevent new child
directories or decrypted sidecars. A preflight PASS protects existing files
only, and is insufficient by itself when all filesystem changes are forbidden.
Keep the recorded one-boot guards; do not treat this as a containment sandbox.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from datetime import datetime, timezone
from audit_validation_content import audit

WORKSPACE = Path('F:/Games/MLB16PS3')
BUILD = WORKSPACE / 'build-mlb16-native-pinned'
OUT = BUILD / 'task021b'
DISC = WORKSPACE / 'game/MLB The Show 16 (USA)'
RPCS3 = WORKSPACE / 'rpcs3emu/dev_hdd0/game/BCUS01089_R'
ROOTS = [WORKSPACE / 'game', RPCS3]
CONTENT = {'disc': DISC / 'PS3_GAME/USRDIR',
           'native_hdd': DISC / 'game/BCUS01089_R/USRDIR',
           'rpcs3_hdd': RPCS3 / 'USRDIR'}
KERNEL = ctypes.WinDLL('kernel32', use_last_error=True)
KERNEL.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                             ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD,
                             wintypes.HANDLE]
KERNEL.CreateFileW.restype = wintypes.HANDLE
KERNEL.CloseHandle.argtypes = [wintypes.HANDLE]
KERNEL.CloseHandle.restype = wintypes.BOOL
INVALID = ctypes.c_void_p(-1).value


def save(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2), encoding='utf-8')


def inventory():
    result = {}
    for root in ROOTS:
        for path in [root, *sorted(root.rglob('*'))]:
            if path.lstat().st_file_attributes & 0x400:  # FILE_ATTRIBUTE_REPARSE_POINT
                raise RuntimeError(f'Refusing a reparse path: {path}')
            stat = path.stat()
            result[str(path)] = {'directory': path.is_dir(), 'size': stat.st_size,
                                 'mtime_ns': stat.st_mtime_ns}
    return result


def open_handle(path, access):
    # OPEN_EXISTING only: no creation or truncation. Directories need BACKUP_SEMANTICS.
    return KERNEL.CreateFileW(str(path), access, 1, None, 3, 0x02000000, None)


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    global OUT, ROOTS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check-only', action='store_true')
    parser.add_argument('--task022', action='store_true')
    args = parser.parse_args()
    task = '022' if args.task022 else '021B'
    hdd_root = RPCS3.parents[1] if args.task022 else DISC
    if args.task022:
        OUT = BUILD / 'task022'
        ROOTS = [WORKSPACE / 'game', hdd_root]
    trace_env = f'MLB16_TASK{task}_TRACE'
    OUT.mkdir(exist_ok=True)
    if not args.check_only:
        if (OUT / 'boot-result.json').exists() or (OUT / 'boot-started.json').exists():
            raise RuntimeError('One-boot guard: this experiment has already launched')
        gate = json.loads((OUT / 'build-gate.json').read_text())
        if (not gate['ready'] or gate.get('task', task) != task or
                gate['exe_sha256'] != sha256(BUILD / 'MyGameRecomp.exe')):
            raise RuntimeError('Missing green build gate or executable changed')
        running = subprocess.run(['tasklist.exe', '/FI', 'IMAGENAME eq MyGameRecomp.exe',
                                  '/FO', 'CSV', '/NH'], capture_output=True, text=True, check=True)
        if '"MyGameRecomp.exe"' in running.stdout:
            raise RuntimeError('Refusing boot while another native runner exists')
    before = inventory()
    handles = []
    try:
        for path in before:
            handle = open_handle(path, 0x80000000)  # GENERIC_READ, FILE_SHARE_READ only
            if handle == INVALID:
                raise ctypes.WinError(ctypes.get_last_error())
            handles.append(handle)
        checks = []
        for root in CONTENT.values():
            path = root / 'MLBPS3_GD.WAD'
            for access in (0x40000000, 0x00010000):  # write access / DELETE access
                handle = open_handle(path, access)
                error = ctypes.get_last_error()
                if handle != INVALID:
                    KERNEL.CloseHandle(handle)
                    raise RuntimeError(f'Protection did not deny access {access:X} to {path}')
                if error != 32:  # ERROR_SHARING_VIOLATION
                    raise RuntimeError(f'Unexpected protection error {error}: {path}')
                checks.append({'path': str(path), 'access': hex(access), 'denied_error': error})
        # A read open and full audit must still succeed under these sharing locks.
        before_audit = audit(CONTENT)
        save('protection-check.json', {'held_handles': len(handles), 'checks': checks,
                                      'existing_files_write_delete_denied': True})
        save('data-before.json', before)
        save('content-before.json', before_audit)
        if not args.check_only:
            env = dict(os.environ)
            prefixes = ('SPU_', 'SPURS_', 'YDKJ_', 'LBP_', 'GH3_', 'MLB16_TASK',
                        'FS_', 'PS3_FS', 'PS3_HDD', 'PS3_VFS', 'EDAT_')
            removed = sorted(key for key in env if key.upper().startswith(prefixes))
            env = {key: value for key, value in env.items() if key not in removed}
            stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
            log = OUT / f'boot-{stamp}.log'
            body = f'''cd /f/Games/MLB16PS3/build-mlb16-native-pinned || exit 2
export PATH=/clang64/bin:/usr/bin:$PATH
export MSYSTEM=CLANG64
export TEMP='{OUT.as_posix()}' TMP='{OUT.as_posix()}'
export PS3_VFS_ROOT='{DISC.as_posix()}'
export PS3_HDD0_ROOT='{hdd_root.as_posix()}'
export {trace_env}=1
timeout -k 5s 45s /f/Games/MLB16PS3/build-mlb16-native-pinned/MyGameRecomp.exe 'F:/Games/MLB16PS3/game/EBOOT.ELF' > '{log.as_posix()}' 2>&1
boot_rc=$?
unset {trace_env}
printf 'boot_exit=%s {trace_env}=%s\\n' "$boot_rc" "${{{trace_env}-unset}}"
exit "$boot_rc"
'''
            script = OUT / 'boot.sh'
            script.write_text(body, encoding='utf-8', newline='\n')
            argv = ['D:/APP/msys64/usr/bin/bash.exe', '--noprofile', '--norc', str(script)]
            save('boot-started.json', {'argv': argv, 'script': body, 'removed_env': removed,
                                       'log': str(log), 'held_handles': len(handles)})
            start = time.monotonic()
            process = subprocess.run(argv, env=env, capture_output=True, text=True)
            save('boot-result.json', {'exit': process.returncode, 'seconds': time.monotonic() - start,
                                      'stdout': process.stdout, 'stderr': process.stderr, 'log': str(log)})
            # A timed-out native child must be gone before releasing data protection.
            running = subprocess.run(['tasklist.exe', '/FI', 'IMAGENAME eq MyGameRecomp.exe',
                                      '/FO', 'CSV', '/NH'], capture_output=True, text=True)
            if '"MyGameRecomp.exe"' in running.stdout:
                subprocess.run(['taskkill.exe', '/F', '/T', '/IM', 'MyGameRecomp.exe'],
                               check=True, capture_output=True)
                raise RuntimeError('Native child survived timeout; terminated before unlocking')
        after_audit = audit(CONTENT)
        after = inventory()
        save('content-after.json', after_audit)
        save('data-after.json', after)
        same = before == after and before_audit == after_audit
        save('preservation.json', {'inventory_unchanged': before == after,
                                  'six_hashes_metadata_unchanged': before_audit == after_audit,
                                  'held_handles': len(handles), 'check_only': args.check_only})
        if not same:
            raise RuntimeError('Preservation comparison failed; inspect local manifests')
        print(json.dumps({'check_only': args.check_only, 'protection': 'PASS',
                          'preservation': 'PASS', 'held_handles': len(handles)}))
    finally:
        for handle in reversed(handles):
            KERNEL.CloseHandle(handle)


if __name__ == '__main__':
    main()
