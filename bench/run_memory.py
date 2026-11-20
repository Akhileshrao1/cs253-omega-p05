"""Compile and record the D7 structure, input, and process memory measurements."""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from bench.run import command_output, host_details  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, default=1_000_000)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument(
        "--out", type=Path,
        default=ROOT / "results" / "raw" / "memory_footprint.csv",
    )
    args = parser.parse_args()
    if args.n <= 0:
        parser.error("--n must be positive")

    output = args.out if args.out.is_absolute() else ROOT / args.out
    metadata_output = output.with_suffix(".environment.json")
    if output.exists() or metadata_output.exists():
        parser.error(
            f"refusing to overwrite {output} or {metadata_output}; "
            "choose a new --out path"
        )

    build_dir = ROOT / "build" / "benchmark"
    build_dir.mkdir(parents=True, exist_ok=True)
    binary = build_dir / ("memory_footprint.exe" if os.name == "nt" else "memory_footprint")
    source = ROOT / "bench" / "memory_footprint.cpp"
    started = datetime.now(timezone.utc).isoformat(timespec="seconds")
    compile_command = [
        args.cxx, "-std=c++17", "-O2", "-Wall", "-Wextra", "-pedantic",
        str(source), "-o", str(binary),
    ]
    subprocess.run(compile_command, cwd=ROOT, check=True)

    run_command = [str(binary), str(args.n)]
    completed = subprocess.run(
        run_command, cwd=ROOT, check=True, capture_output=True, text=True,
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(completed.stdout, encoding="utf-8")

    metadata = {
        "started_utc": started,
        "n": args.n,
        "input_seed": None,
        "input_pattern": "deterministic: readings[i]=i%100003; calibration[i]=i%1000000001",
        "invocation": [sys.executable, *sys.argv],
        "compile_command": compile_command,
        "measurement_command": run_command,
        "measurement_notes": completed.stderr.strip(),
        "compiler_path": shutil.which(args.cxx),
        "compiler_version": command_output(args.cxx, "--version"),
        "host": host_details(),
        "python": sys.version,
        "csv_path": str(output.relative_to(ROOT)) if output.is_relative_to(ROOT) else str(output),
        "csv_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
    }
    metadata_output.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(f"Memory CSV: {output}")
    print(f"Run metadata: {metadata_output}")


if __name__ == "__main__":
    main()
