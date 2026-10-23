"""Reconstruct the audited toolkit offline, outside repositories/build trees.

Uses only existing Git objects and reviewed patches. Never clones, initializes
Git, stages, commits, changes remotes, builds, or boots anything.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import zipfile


def git(source, *args):
    return subprocess.run(
        ["git", "-C", str(source), "-c", "core.autocrlf=false", *args],
        check=True, capture_output=True,
    ).stdout


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True,
                        help="Any Git checkout containing the exact upstream base")
    parser.add_argument("--output", type=Path, required=True,
                        help="A new directory under the workspace's tmp directory")
    parser.add_argument("--profile", choices=("persistent", "task017"), default="task017")
    parser.add_argument("--compare", type=Path,
                        help="Optionally byte-compare every upstream-tracked file")
    args = parser.parse_args()
    port = Path(__file__).resolve().parents[1]
    source, output = args.source.resolve(), args.output.resolve()
    scratch_root = (port.parent / "tmp").resolve()
    if not output.is_relative_to(scratch_root) or output == scratch_root:
        parser.error("--output must be a new child of " + str(scratch_root))
    if output.is_relative_to(source) or source.is_relative_to(output):
        parser.error("--output must be separate from --source")
    if output.exists():
        parser.error("--output already exists; choose a new directory")
    lock = json.loads((port / "dependencies/ps3recomp.lock.json").read_text())
    base = lock["upstream"]["commit"]
    if git(source, "rev-parse", base + "^{commit}").decode().strip() != base:
        raise RuntimeError("Exact upstream commit is unavailable")
    selected = set(lock["profiles"][args.profile])
    changes = [c for c in lock["changes"] if c["patch"] in selected]
    for change in changes:
        if digest((port / change["patch"]).read_bytes()) != change["patch_sha256"]:
            raise RuntimeError("Patch hash mismatch: " + change["patch"])
        if digest(git(source, "show", base + ":" + change["target"])) != change["base_sha256"]:
            raise RuntimeError("Base hash mismatch: " + change["target"])
    # Export existing objects, without copying the dirty checkout or .git.
    archive = git(source, "archive", "--format=zip", base)
    with zipfile.ZipFile(io.BytesIO(archive)) as bundle:
        for entry in bundle.infolist():
            if not (output / entry.filename).resolve().is_relative_to(output):
                raise RuntimeError("Unsafe archive path")
        output.mkdir(parents=True)
        bundle.extractall(output)
    for change in changes:
        patch = str(port / change["patch"])
        git(output, "apply", "--check", patch)
        git(output, "apply", patch)
    expected = {c["target"]: c["patched_sha256"] for c in changes}
    files = git(source, "ls-tree", "-r", "-z", base).split(b"\0")
    compared = 0
    for entry in filter(None, files):
        metadata, encoded_name = entry.split(b"\t", 1)
        mode, kind, oid = metadata.split()
        if kind != b"blob":
            raise RuntimeError("Unexpected nested dependency in upstream archive")
        name = encoded_name.decode("utf-8")
        data = (output / name).read_bytes()
        if name in expected:
            matches = digest(data) == expected[name]
        else:
            blob = b"blob " + str(len(data)).encode() + b"\0" + data
            matches = hashlib.sha1(blob).hexdigest() == oid.decode()
        if not matches:
            raise RuntimeError("Reconstruction mismatch: " + name)
        if args.compare and data != (args.compare.resolve() / name).read_bytes():
            raise RuntimeError("Comparison checkout differs: " + name)
        compared += 1
    print(json.dumps({"base": base, "profile": args.profile,
                      "verified_files": compared, "patches": len(changes),
                      "byte_compared": args.compare is not None,
                      "output": str(output)}, indent=2))


if __name__ == "__main__":
    main()
