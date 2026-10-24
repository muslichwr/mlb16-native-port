"""Normalize TASK-026 ordinary logs and persistent outcomes; no guest execution.

No timestamp/order/host-thread/fd/frequency is a progress key. An unrecoverable
ordinary message is unknown, not proof of a skipped guest call. Full runtime
logs and signatures stay outside Git. --spec freezes comparison rules first.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT = Path('F:/Games/MLB16PS3/build-mlb16-native-pinned/task026')
PATTERNS = {
    'STARTUP_SEMAPHORE_127AC': r'lr=0x000127AC\b',
    'STARTUP_SEMAPHORE_127EC': r'lr=0x000127EC\b',
    'MAIN_WORKER_CREATION': r'name="MainMultiTask"\s+entry=0x010C84F8',
    'SYS_HEADER_READ_RESULT': r'nbytes=16 -> 16 \(magic=7CCA8590',
    'GD_HEADER_READ_RESULT': r'nbytes=65536 -> 65536 \(magic=0116BF20',
    'EXT_HEADER_READ_RESULT': r'nbytes=30280 -> 30280 \(magic=E801C130',
    'AUDIO_PORT_OPEN_REQUEST': r'PortOpen\(nChannel=8, nBlock=8\)',
    'AUDIO_PORT_READY_RESULT': r'GetPortConfig\(port=0\) status=READY',
    'SCREENSHOT_PARAMETER_REQUEST': r'\[cellScreenshot\] SetParameter\(\)',
    'INTRO_FILENAME_REQUEST': r'MLB_INTRO\.BIK',
    'HTTP_CLIENT_REQUEST': r'\[cellHttp\] CreateClient\(id=\d+\)',
}
SPEC = {'version': 1, 'milestone_patterns': PATTERNS,
        'comparison_keys': ['termination_kind_and_natural_or_crash_code',
                            'sys_md5_published_size_and_known_checksum',
                            'corrupt_data_text_observation', 'semantic_event_presence'],
        'excluded': ['timestamps', 'raw_order', 'line_numbers', 'host_thread_ids',
                     'transient_fds', 'event_counts', 'scheduler_sensitive_log_volume',
                     'driver_profile_caches', 'directory_mtimes', 'elapsed_timeout_duration'],
        'absence_rule': 'No ordinary log match means NOT_RECOVERABLE, not non-execution.',
        'difference_rule': 'Termination or verified SYS.MD5 outcome differences prove a category; '
                           'unequal recoverable log-event sets alone are inconclusive.',
        'scope': 'Stable ordinary-runtime observability only; no guest startup/dispatcher return probe.'}


def read(path):
    return json.loads(path.read_text())


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True), encoding='utf-8')


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def analyze(number):
    run = ROOT / f'run{number}'
    text = (run / 'boot.log').read_text(errors='replace')
    milestones, locators = {}, {}
    for name, pattern in PATTERNS.items():
        found = re.search(pattern, text)
        milestones[name] = 'OBSERVED' if found else 'NOT_RECOVERABLE'
        if found:
            locators[name] = {'line': text.count('\n', 0, found.start()) + 1,
                              'matched_text': found[0]}
    # The original runtime's interleaving can damage words; never invent a missing dialog.
    corrupt = re.search(r'(?:game\s*data[^\n]{0,160}corrupt|corrupt[^\n]{0,160}game\s*data)', text, re.I)
    dialog = 'CORRUPT_GAME_DATA_TEXT_OBSERVED' if corrupt else 'NO_CORRUPT_DATA_DIALOG_OBSERVED'
    if corrupt:
        locators['CORRUPT_DIALOG_TEXT'] = {'line': text.count('\n', 0, corrupt.start()) + 1,
                                          'matched_text': corrupt[0]}
    final = read(run / 'final-guest-inputs.json')
    cache = final.get('hdd1/cache/BCUS01089/MLBPS3_SYS.MD5')
    expected = read(ROOT / 'seed-summary.json')['expected_sys_md5_sha256']
    cache_result = {'published': cache is not None,
                    'size': cache.get('size') if cache else None,
                    'matches_disc_sys_header_first16': bool(cache and cache.get('size') == 16 and cache.get('sha256') == expected)}
    termination = read(run / 'boot-result.json')
    signature = {'termination': termination['termination'],
                 'exit_code': None if termination['termination'] == 'FAIL_SAFE_TIMEOUT' else termination['exit_code'],
                 'corrupt_data_outcome': dialog, 'sys_md5': cache_result, 'milestones': milestones}
    mutation = read(run / 'sandbox-mutations.json')
    # Full details remain evidence, never a scheduler-sensitive event-count signature.
    outputs = {k: {'size': v.get('size'), 'sha256': v.get('sha256')}
               for k, v in mutation['added'].items()
               if not v['directory'] and k.startswith(('hdd0/', 'hdd1/', 'vfs/'))}
    result = {'run': number, 'signature': signature, 'signature_sha256': digest(signature),
              'locators': locators, 'published_guest_files': outputs,
              'mutations': {k: len(v) for k, v in mutation.items()},
              'cache_directories': sorted(k for k, v in final.items() if v['directory'] and k.startswith('hdd1/cache/')),
              'limitations': ['ordinary-message absence is not a negative guest-execution observation',
                              'no task-specific startup/dispatcher/validation result observer enabled']}
    save(run / 'normalized-signature.json', result)
    return result


def compare():
    if read(ROOT / 'comparison-specification.json') != SPEC:
        raise RuntimeError('Comparison specification changed after freezing')
    runs = [analyze(i) for i in (1, 2, 3)]
    gates = [read(ROOT / f'run{i}/launch-equivalence.json') for i in (1, 2, 3)]
    if not all(g == gates[0] and g['PASS'] for g in gates):
        raise RuntimeError('Launch inputs were not equivalent')
    if not all(read(ROOT / f'run{i}/preservation.json')['all_original_hashes_and_metadata_unchanged'] for i in (1, 2, 3)):
        raise RuntimeError('Original-data audit failed')
    sig = [r['signature'] for r in runs]
    category = None
    if len({(s['termination'], s['exit_code']) for s in sig}) != 1:
        classification, category = 'NATIVE_BOOT_NONDETERMINISTIC', 'TERMINATION_BEHAVIOR'
    elif len({digest(s['sys_md5']) for s in sig}) != 1:
        classification, category = 'NATIVE_BOOT_NONDETERMINISTIC', 'SYS_CACHE_PUBLICATION_OUTCOME'
    elif len({digest(s) for s in sig}) == 1:
        classification = 'NATIVE_BOOT_REPRODUCIBLE'
    else:
        classification = 'BASELINE_COMPARISON_INCONCLUSIVE'
    common = [k for k in PATTERNS if all(s['milestones'][k] == 'OBSERVED' for s in sig)]
    summary = {'classification': classification, 'earliest_proven_divergence_category': category,
               'common_observed_milestones': common,
               'unequal_observation_keys': [k for k in PATTERNS if len({s['milestones'][k] for s in sig}) != 1],
               'signatures': [r['signature_sha256'] for r in runs],
               'matched_launch_inputs': True, 'originals_unchanged_after_every_run': True,
               'scope': 'Three bounded boots at the available non-instrumented observability level.'}
    save(ROOT / 'cross-run-comparison.json', summary)
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--spec', action='store_true')
    args = parser.parse_args()
    if args.spec:
        if (ROOT / 'comparison-specification.json').exists():
            raise RuntimeError('Comparison specification already frozen')
        save(ROOT / 'comparison-specification.json', SPEC)
        print('Comparison specification frozen; no game executed')
    else:
        compare()
