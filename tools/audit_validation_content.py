"""Read-only comparison of the three GD archive locations. No game boot or writes.

Prints metadata/hashes and verifies whether the native HDD file is an exact
prefix of both complete files. Supply directories containing MLBPS3_GD.WAD.
"""
import argparse
import hashlib
import json
from contextlib import ExitStack
from pathlib import Path


def metadata(path):
    stat = path.stat()
    return {"size": stat.st_size, "mtime_ns": stat.st_mtime_ns}


def audit(roots):
    result = {"files": {}, "native_prefix_matches": True}
    for name in ("MLBPS3_GD.WAD.HDR", "MLBPS3_GD.WAD"):
        paths = {key: root / name for key, root in roots.items()}
        before = {key: metadata(path) for key, path in paths.items()}
        hashes = {key: hashlib.sha256() for key in paths}
        prefix_equal = True
        complete_equal = True
        with ExitStack() as stack:
            files = {key: stack.enter_context(path.open("rb")) for key, path in paths.items()}
            while True:
                blocks = {key: stream.read(1024 * 1024) for key, stream in files.items()}
                if not any(blocks.values()):
                    break
                for key, block in blocks.items():
                    hashes[key].update(block)
                native = blocks["native_hdd"]
                if native:
                    prefix_equal &= all(blocks[key][:len(native)] == native for key in ("disc", "rpcs3_hdd"))
                complete_equal &= blocks["disc"] == blocks["rpcs3_hdd"]
        after = {key: metadata(path) for key, path in paths.items()}
        if before != after:
            raise RuntimeError("File metadata changed during read-only audit")
        result["files"][name] = {
            key: {"path": str(paths[key].resolve()), **before[key], "sha256": hashes[key].hexdigest()}
            for key in paths
        }
        result["files"][name]["disc_equals_rpcs3"] = complete_equal
        result["files"][name]["native_is_prefix_of_both"] = prefix_equal
        result["native_prefix_matches"] &= prefix_equal
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", required=True, type=Path)
    parser.add_argument("--native-hdd", required=True, type=Path)
    parser.add_argument("--rpcs3-hdd", required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(audit({"disc": args.disc, "native_hdd": args.native_hdd, "rpcs3_hdd": args.rpcs3_hdd}), indent=2))
