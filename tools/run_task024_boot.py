"""TASK-024: fresh sandbox, unchanged TASK-022R isolation and original audit."""
import argparse
import run_task022r_boot as isolated

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--prepare', action='store_true')
    mode.add_argument('--boot', action='store_true')
    args = parser.parse_args()
    isolated.ROOT = isolated.protection.BUILD / 'task024'
    isolated.VFS, isolated.HDD0, isolated.HDD1 = (
        isolated.ROOT / name for name in ('vfs', 'hdd0', 'hdd1'))
    isolated.TRACE_ENV = 'MLB16_TASK024_TRACE'
    isolated.ROOT.mkdir(exist_ok=True)
    isolated.prepare() if args.prepare else isolated.boot()
