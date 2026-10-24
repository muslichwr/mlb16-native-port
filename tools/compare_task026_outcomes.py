"""Adjudicate TASK-026 using normalized logs AND audited installed-file outcomes.

The ordinary-log normalizer is deliberately conservative about missing text.
This adds positive disk-state evidence: compare presence/type/size of every
original installed seed file after termination. Ignore mtimes, same-size hash
changes, new transient cache files, profiles and log volume. No guest runs.
"""
import json
from pathlib import Path
import analyze_task026_baseline as logs


def compare():
    logs.compare()
    root = logs.ROOT
    normal = logs.read(root / 'cross-run-comparison.json')
    seed = logs.read(root / 'seed-manifest.json')
    shapes = []
    for number in (1, 2, 3):
        final = logs.read(root / f'run{number}/final-guest-inputs.json')
        shape = {}
        for rel, initial in seed.items():
            if initial['directory']:
                continue
            value = final.get('hdd0/game/BCUS01089_R/' + rel)
            shape[rel] = {'present': value is not None,
                          'directory': value['directory'] if value else None,
                          'size': value.get('size') if value else None}
        shapes.append(shape)
        result = logs.read(root / f'run{number}/normalized-signature.json')
        full = {'ordinary': result['signature'], 'installed_file_shape': shape}
        logs.save(root / f'run{number}/full-outcome-signature.json',
                  {'signature': full, 'signature_sha256': logs.digest(full),
                   'excludes': ['mtime', 'same-size content hash changes', 'new transient caches',
                                'profiles', 'timestamps', 'thread_ids', 'fds', 'order', 'volume']})
    differences = {p: [s[p] for s in shapes] for p in shapes[0]
                   if any(s[p] != shapes[0][p] for s in shapes[1:])}
    if differences:
        classification = 'NATIVE_BOOT_NONDETERMINISTIC'
        category = 'PERSISTENT_INSTALLED_FILE_STATE'
        basis = 'Identical prelaunch seed/executable/environment/input manifests; '
        basis += 'post-termination installed seed-file presence/type/size differs. '
        basis += 'This is audited disk evidence, not absence of an ordinary log record.'
    else:
        classification = normal['classification']
        category = normal['earliest_proven_divergence_category']
        basis = 'No installed-file structural difference; use the conservative ordinary-outcome comparison.'
    result = {'classification': classification, 'earliest_proven_divergence_category': category,
              'basis': basis, 'installed_file_structural_differences': differences,
              'matched_launch_inputs': normal['matched_launch_inputs'],
              'originals_unchanged_after_every_run': normal['originals_unchanged_after_every_run'],
              'ordinary_signature_sha256': normal['signatures'],
              'full_outcome_signature_sha256': [logs.read(root / f'run{i}/full-outcome-signature.json')['signature_sha256']
                                               for i in (1, 2, 3)],
              'limitation': 'Category only. Exact earliest guest instruction, writer, completion, '
                            'subsystem, cause and original-data corruption are not established.'}
    logs.save(root / 'full-baseline-comparison.json', result)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    compare()
