"""TASK-022R: isolated writable mounts, file-only disc links, original-file locks.

--prepare seeds exactly four installed files and demonstrates path isolation.
--boot rechecks the gate and runs one canonical executable with a 180s limit.
All output and writable directories are beneath canonical-build/task022r.
No original directory is aliased, altered, cleaned, or used as a writable root.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
from contextlib import contextmanager
import run_task021b_boot as protection

ROOT = protection.BUILD / 'task022r'
TRACE_ENV = 'MLB16_TASK022R_TRACE'
VFS, HDD0, HDD1 = (ROOT / name for name in ('vfs', 'hdd0', 'hdd1'))
ORIGINALS = [protection.WORKSPACE / 'game', protection.WORKSPACE / 'rpcs3emu/dev_hdd0']
INSTALLED = protection.RPCS3 / 'USRDIR'
SEEDS = ['MLBPS3_GD.WAD', 'MLBPS3_GD.WAD.HDR', 'MLBPS3_EXT.WAD', 'MLBPS3_EXT.WAD.HDR']


def save(name, data):
    (ROOT / name).write_text(json.dumps(data, indent=2), encoding='utf-8')


def inside(path, root=None):
    if root is None:
        root = ROOT
    return path.resolve().is_relative_to(root.resolve())


def manifest(hashes=False):
    protection.ROOTS = ORIGINALS
    result = protection.inventory()
    for name, value in result.items():
        stat = Path(name).stat()
        value.update(inode=stat.st_ino, links=stat.st_nlink, attributes=stat.st_file_attributes)
        if hashes and not value['directory']:
            value['sha256'] = protection.sha256(Path(name))
    return result


@contextmanager
def locked():
    handles = []
    try:
        for path in manifest():
            handle = protection.open_handle(path, 0x80000000)
            if handle == protection.INVALID:
                raise protection.ctypes.WinError(protection.ctypes.get_last_error())
            handles.append(handle)
        yield len(handles)
    finally:
        for handle in reversed(handles):
            protection.KERNEL.CloseHandle(handle)


def environment():
    prefixes = ('SPU_', 'SPURS_', 'YDKJ_', 'LBP_', 'GH3_', 'MLB16_TASK', 'FS_', 'PS3_', 'EDAT_')
    env = {k: v for k, v in os.environ.items() if not k.upper().startswith(prefixes)}
    env.update(PS3_VFS_ROOT=str(VFS), PS3_HDD0_ROOT=str(HDD0), PS3_HDD1_ROOT=str(HDD1),
               PS3_DEV_FLASH=str(ROOT / 'dev_flash'), TEMP=str(ROOT / 'tmp'), TMP=str(ROOT / 'tmp'),
               APPDATA=str(ROOT / 'profile/AppData/Roaming'),
               LOCALAPPDATA=str(ROOT / 'profile/AppData/Local'), USERPROFILE=str(ROOT / 'profile'))
    env[TRACE_ENV] = '1'
    env['PATH'] = 'D:/APP/msys64/clang64/bin;D:/APP/msys64/usr/bin;' + env['PATH']
    return env


def experiment_inventory():
    result = {}
    for path in ROOT.rglob('*'):
        stat = path.lstat()
        result[str(path.relative_to(ROOT))] = {'directory': path.is_dir(), 'link': path.is_symlink(),
                                              'size': stat.st_size, 'mtime_ns': stat.st_mtime_ns}
    return result


def unmount_disc_links():
    """Remove only our verified file symlinks, never their original targets."""
    removed = []
    for entry in json.loads((ROOT / 'disc-file-links.json').read_text()):
        path = Path(entry['link'])
        if not inside(path.parent) or not path.is_symlink() or path.is_dir():
            raise RuntimeError(f'Unsafe disc unlink: {path}')
        if path.resolve(strict=True) != Path(entry['target']).resolve(strict=True):
            raise RuntimeError(f'Changed disc link: {path}')
        path.unlink()
        removed.append(str(path))
    save('mirror-unmounted.json', {'removed_file_links': removed, 'original_targets_deleted': False})


def preflight():
    directory_count = link_count = 0
    for path in [ROOT, *ROOT.rglob('*')]:
        if path.is_dir():
            # Never follow an original directory alias: all parents must be real.
            if path.lstat().st_file_attributes & 0x400 or not inside(path):
                raise RuntimeError(f'Directory escapes experiment: {path}')
            directory_count += 1
        elif path.is_symlink():
            target = path.resolve(strict=True)
            if not target.is_file():
                raise RuntimeError(f'Non-file link: {path}')
            if not (inside(target) or target.is_relative_to(protection.DISC)):
                raise RuntimeError(f'Unexpected link target: {target}')
            link_count += 1
    for name in SEEDS:
        dest = HDD0 / 'game/BCUS01089_R/USRDIR' / name
        if not inside(dest) or dest.is_symlink() or dest.stat().st_ino == (INSTALLED / name).stat().st_ino:
            raise RuntimeError('Installed seed is not an independent experiment file')
        if protection.sha256(dest) != protection.sha256(INSTALLED / name):
            raise RuntimeError(f'Seed hash mismatch: {name}')
    probe = ROOT / 'validation_file_replay.exe'
    process = subprocess.run([str(probe), str(VFS), '--map-only'], env=environment(),
                             cwd=ROOT, capture_output=True, text=True, check=True)
    maps = dict(line.split('|')[1:] for line in process.stdout.splitlines() if line.startswith('MAP|'))
    if len(maps) != 3:
        raise RuntimeError('Incomplete native resolver preflight')
    for guest, host in maps.items():
        path = Path(host)
        if guest.startswith('/dev_bdvd/'):
            if not inside(path.parent) or not path.is_symlink():
                raise RuntimeError('Disc write parent is not isolated')
        elif not inside(path):
            raise RuntimeError(f'Writable mapping escapes: {guest} -> {host}')
    denied = []
    for path in [VFS / 'PS3_GAME/USRDIR/MLBPS3_GD.WAD',
                 protection.DISC / 'PS3_GAME/USRDIR/MLBPS3_GD.WAD']:
        handle = protection.open_handle(path, 0x40000000)  # Access only; no write/truncate.
        error = protection.ctypes.get_last_error()
        if handle != protection.INVALID:
            protection.KERNEL.CloseHandle(handle)
            raise RuntimeError(f'Original disc target is writable: {path}')
        if error != 32:
            raise RuntimeError(f'Unexpected write-protection error {error}')
        denied.append({'path': str(path), 'error': error})
    # Actual child creation is tested only under independent experiment parents.
    for parent in [HDD0, HDD1 / 'cache', VFS / 'PS3_GAME/USRDIR']:
        if not inside(parent):
            raise RuntimeError('Unsafe canary parent')
        canary = parent / '.task022r-isolation-canary'
        with canary.open('xb') as stream:
            stream.write(b'isolated')
        canary.unlink()
    return {'PASS': True, 'directory_count': directory_count, 'file_link_count': link_count,
            'maps': maps, 'write_access_denied': denied, 'no_original_directory_aliases': True,
            'independent_seed_copies': True, 'writable_canaries_passed': True}


def prepare():
    if (ROOT / 'prepared.json').exists():
        raise RuntimeError('Preparation already exists; no automatic reseeding')
    with locked() as count:
        before = manifest(hashes=True)
        save('original-before.json', before)
        for path in [VFS, HDD0 / 'game/BCUS01089_R/USRDIR', HDD1 / 'cache', ROOT / 'tmp',
                     ROOT / 'dev_flash', ROOT / 'profile/AppData/Local', ROOT / 'profile/AppData/Roaming']:
            path.mkdir(parents=True, exist_ok=True)
        links = []
        for top in ['PS3_GAME', 'PS3_DISC.SFB', 'PS3UPDAT.PUP']:
            source = protection.DISC / top
            for path in ([source, *source.rglob('*')] if source.is_dir() else [source]):
                if not path.exists():
                    continue
                dest = VFS / path.relative_to(protection.DISC)
                if path.is_dir():
                    dest.mkdir(parents=True, exist_ok=True)
                else:
                    os.symlink(path, dest)  # File only: sidecars stay beside links in VFS.
                    links.append({'link': str(dest), 'target': str(path)})
        save('disc-file-links.json', links)
        seeds = []
        for name in SEEDS:
            source, dest = INSTALLED / name, HDD0 / 'game/BCUS01089_R/USRDIR' / name
            if dest.exists():
                raise RuntimeError('Refusing to overwrite an experiment seed')
            with source.open('rb') as src, dest.open('xb') as dst:
                shutil.copyfileobj(src, dst, 4 * 1024 * 1024)
            seeds.append({'source': str(source), 'destination': str(dest), 'size': dest.stat().st_size})
        save('seeds.json', seeds)
        gate = preflight()
        # Hashes were read before preparation; metadata/identities must be unchanged now.
        assert manifest() == {k: {f: v for f, v in m.items() if f != 'sha256'} for k, m in before.items()}
        save('isolation-preflight.json', gate)
        save('prepared.json', {'held_handles': count, 'isolation': 'PASS'})
        print('PREPARED: isolation PASS', flush=True)


def boot():
    if (ROOT / 'boot-started.json').exists():
        raise RuntimeError('One-boot guard: TASK-022R already launched')
    gate = json.loads((ROOT / 'build-gate.json').read_text())
    exe = protection.BUILD / 'MyGameRecomp.exe'
    assert gate['ready'] and gate['exe_sha256'] == protection.sha256(exe)
    active = subprocess.check_output(['tasklist.exe', '/FI', 'IMAGENAME eq MyGameRecomp.exe', '/FO', 'CSV', '/NH'], text=True)
    assert '"MyGameRecomp.exe"' not in active
    with locked() as count:
        before = manifest(hashes=True)
        assert before == json.loads((ROOT / 'original-before.json').read_text())
        isolation = preflight()
        save('launch-isolation-gate.json', isolation)
        save('experiment-before.json', experiment_inventory())
        env = environment()
        argv = [str(exe), str(protection.WORKSPACE / 'game/EBOOT.ELF')]
        save('boot-started.json', {'argv': argv, 'cwd': str(ROOT), 'fail_safe_seconds': 180,
                                  'mounts': {k: v for k, v in env.items() if k.startswith('PS3_')},
                                  'held_handles': count})
        start = time.monotonic()
        process = None
        try:
            with (ROOT / 'boot.log').open('wb') as log:
                process = subprocess.Popen(argv, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
                try:
                    code = process.wait(timeout=180)
                    reason = 'CRASH' if (code & 0xC0000000) == 0xC0000000 or code in (3, 134) else 'NATURAL_EXIT'
                except subprocess.TimeoutExpired:
                    reason = 'FAIL_SAFE_TIMEOUT'
                    subprocess.run(['taskkill.exe', '/F', '/T', '/PID', str(process.pid)], check=True,
                                   capture_output=True)
                    code = process.wait(timeout=15)
            save('boot-result.json', {'termination': reason, 'exit_code': code,
                                      'seconds': time.monotonic() - start})
        finally:
            if process is not None and process.poll() is None:
                subprocess.run(['taskkill.exe', '/F', '/T', '/PID', str(process.pid)], check=True,
                               capture_output=True)
                process.wait(timeout=15)
            after = manifest(hashes=True)
            save('original-after.json', after)
            save('preservation.json', {'all_original_hashes_and_metadata_unchanged': before == after,
                                       'held_handles': count})
            save('experiment-after.json', experiment_inventory())
            if before != after:
                raise RuntimeError('Original-data preservation failed')
            # Disarm the read-only mirror before releasing original-file locks.
            unmount_disc_links()
        print('BOOT DONE; original preservation PASS', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--prepare', action='store_true')
    mode.add_argument('--boot', action='store_true')
    args = parser.parse_args()
    ROOT.mkdir(exist_ok=True)
    prepare() if args.prepare else boot()
