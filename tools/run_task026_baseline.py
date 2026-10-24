"""TASK-026 host-only full-content baseline; no guest observer or runtime changes.

--prepare freezes inputs and creates three independent copies; --run N launches
that copy once. Outputs,
game copies and full environment records stay under canonical-build/task026.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time
import run_task022r_boot as isolation

protection = isolation.protection
ROOT = protection.BUILD / 'task026'
SOURCE = protection.RPCS3
EXE = protection.BUILD / 'MyGameRecomp.exe'
GUEST_DIRS = ('vfs', 'hdd0', 'hdd1', 'tmp', 'profile', 'dev_flash')
OPTIONS = ('018', '019', '020', '021B', '022', '022R', '023', '024')


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True), encoding='utf-8')


def read(path):
    return json.loads(path.read_text())


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def norm(value, run):
    if isinstance(value, dict):
        return {k: norm(v, run) for k, v in value.items()}
    if isinstance(value, list):
        return [norm(v, run) for v in value]
    if isinstance(value, str):
        return value.replace(str(run), '<SANDBOX>').replace(run.as_posix(), '<SANDBOX>')
    return value


def configure(run):
    isolation.ROOT = run
    isolation.VFS, isolation.HDD0, isolation.HDD1 = (run / n for n in ('vfs', 'hdd0', 'hdd1'))
    # Full-tree integrity is checked here, replacing the old four-file check.
    isolation.SEEDS = []
    isolation.environment = lambda: environment(run)


def environment(run):
    env = read(ROOT / 'frozen-environment.json')
    env.update(PS3_VFS_ROOT=str(run / 'vfs'), PS3_HDD0_ROOT=str(run / 'hdd0'),
               PS3_HDD1_ROOT=str(run / 'hdd1'), PS3_DEV_FLASH=str(run / 'dev_flash'),
               TEMP=str(run / 'tmp'), TMP=str(run / 'tmp'), TMPDIR=str(run / 'tmp'),
               USERPROFILE=str(run / 'profile'),
               APPDATA=str(run / 'profile/AppData/Roaming'),
               LOCALAPPDATA=str(run / 'profile/AppData/Local'))
    if any(k.upper().startswith('MLB16_TASK') for k in env):
        raise RuntimeError('Task diagnostic environment must be absent')
    return env


def build_gate():
    expected = read(ROOT / 'build-gate.json')
    for field, path in [('exe_sha256', EXE),
                        ('cache_sha256', protection.BUILD / 'CMakeCache.txt'),
                        ('ninja_sha256', protection.BUILD / 'build.ninja')]:
        if protection.sha256(path) != expected[field]:
            raise RuntimeError(f'Frozen executable/configuration changed: {path}')
    cache = (protection.BUILD / 'CMakeCache.txt').read_text()
    if not all(f'MLB16_TASK{x}_DIAGNOSTIC:BOOL=OFF' in cache for x in OPTIONS):
        raise RuntimeError('All task diagnostics must be OFF')
    return expected


def seed_from_original(original):
    entries = {}
    for key, value in original.items():
        path = Path(key)
        if path.is_relative_to(SOURCE):
            entries[path.relative_to(SOURCE).as_posix()] = {
                'directory': value['directory'], 'mtime_ns': value['mtime_ns'],
                'read_only': bool(value['attributes'] & 1),
                **({} if value['directory'] else {'size': value['size'], 'sha256': value['sha256']})}
    if '.' not in entries or not any(not v['directory'] for v in entries.values()):
        raise RuntimeError('Incomplete installed seed manifest')
    return entries


def seed_inventory(installed):
    entries, identities = {}, {}
    for path in [installed, *sorted(installed.rglob('*'))]:
        st = path.lstat()
        if st.st_file_attributes & 0x400:
            raise RuntimeError(f'Reparse point in installed copy: {path}')
        rel = path.relative_to(installed).as_posix()
        entries[rel] = {'directory': path.is_dir(), 'mtime_ns': st.st_mtime_ns,
                        'read_only': bool(st.st_file_attributes & 1)}
        if not path.is_dir():
            entries[rel].update(size=st.st_size, sha256=protection.sha256(path))
            identities[rel] = {'inode': st.st_ino, 'links': st.st_nlink}
            if st.st_nlink != 1 or st.st_ino == (SOURCE / rel).stat().st_ino:
                raise RuntimeError(f'Seed copy aliases original: {path}')
    return entries, identities


def mirror(run):
    links = []
    for top in ('PS3_GAME', 'PS3_DISC.SFB', 'PS3UPDAT.PUP'):
        source = protection.DISC / top
        if not source.exists():
            continue
        for path in ([source, *sorted(source.rglob('*'))] if source.is_dir() else [source]):
            dest = run / 'vfs' / path.relative_to(protection.DISC)
            if path.is_dir():
                dest.mkdir(parents=True, exist_ok=True)
            else:
                os.symlink(path, dest)
                links.append({'link': str(dest), 'target': str(path)})
    save(run / 'disc-file-links.json', links)


def guest_inventory(run, original, seeded=None):
    result = {}
    installed = run / 'hdd0/game/BCUS01089_R'
    for domain in GUEST_DIRS:
        parent = run / domain
        for path in [parent, *sorted(parent.rglob('*'))]:
            st = path.lstat()
            rel = path.relative_to(run).as_posix()
            entry = {'directory': path.is_dir(), 'link': path.is_symlink()}
            if path.is_symlink():
                target = path.resolve(strict=True)
                if not target.is_file() or not target.is_relative_to(protection.DISC):
                    raise RuntimeError(f'Unexpected disc link: {path}')
                value = original[str(target)]
                entry.update(target=str(target), size=value['size'], sha256=value['sha256'])
            elif not path.is_dir():
                seed_rel = path.relative_to(installed).as_posix() if path.is_relative_to(installed) else None
                h = seeded[seed_rel]['sha256'] if seeded and seed_rel in seeded else protection.sha256(path)
                entry.update(size=st.st_size, sha256=h, mtime_ns=st.st_mtime_ns,
                             read_only=bool(st.st_file_attributes & 1))
            result[rel] = entry
    return result


def prepare():
    if (ROOT / 'prepared.json').exists() or any((ROOT / f'run{i}').exists() for i in (1, 2, 3)):
        raise RuntimeError('Fresh-root gate: refusing to reseed or reuse sandboxes')
    gate = build_gate()
    prefixes = ('SPU_', 'SPURS_', 'YDKJ_', 'LBP_', 'GH3_', 'MLB16_TASK', 'FS_', 'PS3_', 'EDAT_')
    env = {k: v for k, v in os.environ.items() if not k.upper().startswith(prefixes)}
    env['PATH'] = 'D:/APP/msys64/clang64/bin;D:/APP/msys64/usr/bin;' + env['PATH']
    save(ROOT / 'frozen-environment.json', env)  # Local only; never print environment secrets.
    print('PREPARE: protecting and hashing original trees', flush=True)
    with isolation.locked() as count:
        original = isolation.manifest(hashes=True)
        save(ROOT / 'original-baseline.json', original)
        seed = seed_from_original(original)
        save(ROOT / 'seed-manifest.json', seed)
        with (protection.DISC / 'PS3_GAME/USRDIR/MLBPS3_SYS.WAD.HDR').open('rb') as stream:
            expected_cache = hashlib.sha256(stream.read(16)).hexdigest()
        save(ROOT / 'seed-summary.json', {'source': str(SOURCE), 'manifest_sha256': digest(seed),
             'entries': len(seed), 'files': sum(not v['directory'] for v in seed.values()),
             'bytes': sum(v.get('size', 0) for v in seed.values()), 'expected_sys_md5_sha256': expected_cache})
        for number in (1, 2, 3):
            run = ROOT / f'run{number}'
            run.mkdir(exist_ok=False)
            configure(run)
            for name in GUEST_DIRS:
                (run / name).mkdir()
            (run / 'hdd1/cache').mkdir()
            for name in ('Local', 'Roaming'):
                (run / 'profile/AppData' / name).mkdir(parents=True, exist_ok=True)
            installed = run / 'hdd0/game/BCUS01089_R'
            installed.mkdir(parents=True)
            print(f'PREPARE run{number}: copying COMPLETE installation', flush=True)
            identities = {}
            for rel, entry in sorted(seed.items()):
                source, dest = SOURCE / rel, installed / rel
                if entry['directory']:
                    dest.mkdir(parents=True, exist_ok=True)
                else:
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    sha = hashlib.sha256()
                    with source.open('rb') as src, dest.open('xb') as dst:
                        while block := src.read(4 * 1024 * 1024):
                            sha.update(block)
                            dst.write(block)
                    if sha.hexdigest() != entry['sha256']:
                        raise RuntimeError(f'Seed copy hash mismatch: {rel}')
                    shutil.copystat(source, dest, follow_symlinks=False)
                    st = dest.stat()
                    if st.st_ino == source.stat().st_ino or st.st_nlink != 1:
                        raise RuntimeError('Installed copy is not independent')
                    identities[rel] = {'inode': st.st_ino, 'links': st.st_nlink}
            for rel in sorted((p for p, v in seed.items() if v['directory']), key=len, reverse=True):
                shutil.copystat(SOURCE / rel, installed / rel, follow_symlinks=False)
            save(run / 'seed-manifest.json', seed)
            save(run / 'seed-identities.json', identities)
            mirror(run)
            probe = protection.BUILD / 'task024/validation_file_replay.exe'
            shutil.copyfile(probe, run / 'validation_file_replay.exe')
            save(run / 'resolver-probe.json', {'source': str(probe), 'sha256': protection.sha256(probe)})
            preflight = isolation.preflight()
            save(run / 'isolation-preflight.json', preflight)
            full_env = environment(run)
            config = norm({'environment': full_env, 'cwd': str(run), 'argv': [str(EXE),
                           str(protection.WORKSPACE / 'game/EBOOT.ELF')], 'fail_safe_seconds': 180,
                           'seed_manifest_sha256': digest(seed), 'exe_sha256': gate['exe_sha256']}, run)
            save(run / 'normalized-environment-config.json', config)
            initial = guest_inventory(run, original, seed)
            save(run / 'initial-guest-inputs.json', initial)
            if number == 1:
                save(ROOT / 'normalized-environment-config.json', config)
                save(ROOT / 'initial-guest-inputs.json', initial)
            if config != read(ROOT / 'normalized-environment-config.json') or initial != read(ROOT / 'initial-guest-inputs.json'):
                raise RuntimeError('Initial sandbox/configuration equivalence failed')
            print(f'PREPARE run{number}: isolation and normalized inputs PASS', flush=True)
        ids = [read(ROOT / f'run{i}/seed-identities.json') for i in (1, 2, 3)]
        if any(len({x[p]['inode'] for x in ids}) != 3 for p in ids[0]):
            raise RuntimeError('Sandbox seed copies share a file identity')
        after = isolation.manifest()
        if after != {k: {f: v for f, v in m.items() if f != 'sha256'} for k, m in original.items()}:
            raise RuntimeError('Original metadata changed during preparation')
        save(ROOT / 'prepared.json', {'three_independent_full_copies': True, 'held_handles': count,
             'seed_manifest_sha256': digest(seed), 'environment_config_sha256': digest(config),
             'initial_guest_inputs_sha256': digest(initial), 'build_gate': gate})
    print('PREPARE COMPLETE; no game boot performed', flush=True)


def run_boot(number):
    run = ROOT / f'run{number}'
    if (run / 'boot-started.json').exists():
        raise RuntimeError('One-boot guard: refusing to reuse this sandbox')
    if number > 1 and not read(ROOT / f'run{number-1}/preservation.json')['all_original_hashes_and_metadata_unchanged']:
        raise RuntimeError('Previous original-data audit must pass first')
    prepared = read(ROOT / 'prepared.json')
    gate = build_gate()
    active = subprocess.check_output(['tasklist.exe', '/FI', 'IMAGENAME eq MyGameRecomp.exe', '/FO', 'CSV', '/NH'], text=True)
    if '"MyGameRecomp.exe"' in active:
        raise RuntimeError('Another native game runner exists')
    configure(run)
    print(f'RUN {number}: prelaunch original/seed/content/environment verification', flush=True)
    with isolation.locked() as count:
        before = isolation.manifest(hashes=True)
        if before != read(ROOT / 'original-baseline.json'):
            raise RuntimeError('Original content changed since immutable baseline')
        save(run / 'original-before.json', before)
        seed, ids = seed_inventory(run / 'hdd0/game/BCUS01089_R')
        if seed != read(ROOT / 'seed-manifest.json') or ids != read(run / 'seed-identities.json'):
            raise RuntimeError('Installed copy differs from complete immutable seed')
        full_env = environment(run)
        config = norm({'environment': full_env, 'cwd': str(run), 'argv': [str(EXE),
                       str(protection.WORKSPACE / 'game/EBOOT.ELF')], 'fail_safe_seconds': 180,
                       'seed_manifest_sha256': digest(seed), 'exe_sha256': gate['exe_sha256']}, run)
        if config != read(ROOT / 'normalized-environment-config.json'):
            raise RuntimeError('Normalized launch configuration mismatch')
        isolation_gate = isolation.preflight()
        initial = guest_inventory(run, before, seed)
        if initial != read(ROOT / 'initial-guest-inputs.json'):
            raise RuntimeError('Disposable guest inputs changed before launch')
        save(run / 'launch-isolation-gate.json', isolation_gate)
        save(run / 'launch-equivalence.json', {'PASS': True, 'seed_manifest_sha256': digest(seed),
             'exe_sha256': gate['exe_sha256'], 'environment_config_sha256': digest(config),
             'initial_guest_inputs_sha256': digest(initial), 'held_handles': count})
        argv = [str(EXE), str(protection.WORKSPACE / 'game/EBOOT.ELF')]
        save(run / 'boot-started.json', {'argv': argv, 'cwd': str(run), 'fail_safe_seconds': 180,
             'environment_config_sha256': digest(config), 'exe_sha256': gate['exe_sha256']})
        process = None
        start = time.monotonic()
        print(f'RUN {number}: gates PASS; launching sole 180-second boot', flush=True)
        try:
            with (run / 'boot.log').open('xb') as out:
                process = subprocess.Popen(argv, cwd=run, env=full_env, stdout=out, stderr=subprocess.STDOUT)
                try:
                    code = process.wait(timeout=180)
                    reason = 'CRASH' if (code & 0xC0000000) == 0xC0000000 or code in (3, 134) else 'NATURAL_EXIT'
                except subprocess.TimeoutExpired:
                    reason = 'FAIL_SAFE_TIMEOUT'
                    subprocess.run(['taskkill.exe', '/F', '/T', '/PID', str(process.pid)], check=True, capture_output=True)
                    code = process.wait(timeout=15)
            save(run / 'boot-result.json', {'termination': reason, 'exit_code': code,
                 'seconds': time.monotonic() - start, 'exe_sha256': protection.sha256(EXE)})
            print(f'RUN {number}: {reason}; auditing originals and sandbox mutations', flush=True)
        finally:
            if process is not None and process.poll() is None:
                subprocess.run(['taskkill.exe', '/F', '/T', '/PID', str(process.pid)], check=True, capture_output=True)
                process.wait(timeout=15)
            try:
                after = isolation.manifest(hashes=True)
                save(run / 'original-after.json', after)
                same = before == after
                save(run / 'preservation.json', {'all_original_hashes_and_metadata_unchanged': same,
                     'held_handles': count, 'files': sum(not v['directory'] for v in after.values())})
                final = guest_inventory(run, after)
                save(run / 'final-guest-inputs.json', final)
                mutations = {'added': {k: v for k, v in final.items() if k not in initial},
                             'changed': {k: {'before': initial[k], 'after': v} for k, v in final.items()
                                         if k in initial and v != initial[k]},
                             'removed': {k: v for k, v in initial.items() if k not in final}}
                save(run / 'sandbox-mutations.json', mutations)
            finally:
                isolation.unmount_disc_links()
            if not same:
                raise RuntimeError('Original preservation failed; no further run allowed')
        build_gate()
    print(f'RUN {number} COMPLETE; original-data preservation PASS', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--prepare', action='store_true')
    action.add_argument('--run', type=int, choices=(1, 2, 3))
    args = parser.parse_args()
    prepare() if args.prepare else run_boot(args.run)
