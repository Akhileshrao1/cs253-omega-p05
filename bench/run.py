"""Build, test and reproduce the range-structure measurements on Windows or Linux."""

import argparse
import csv
import ctypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "benchmark"
SOURCES = ["bench/benchmark.cpp", "oracle/oracle.cpp"]
FLAGS = ["-std=c++17", "-O3", "-DNDEBUG", "-Wall", "-Wextra", "-pedantic"]
SEED = 20261119
RUNS = 7


def command_output(*command):
    try:
        return subprocess.check_output(command, cwd=ROOT, stderr=subprocess.DEVNULL,
                                       encoding="utf-8", errors="replace", timeout=10).strip()
    except (OSError, subprocess.SubprocessError):
        return None


def read_text(path):
    try:
        return Path(path).read_text(encoding="utf-8").strip()
    except OSError:
        return None


def host_details():
    details = {
        "os": platform.platform(), "architecture": platform.machine(),
        "cpu": platform.processor(), "logical_cpus": os.cpu_count(),
        "physical_cores": None, "ram_bytes": None, "model": None,
        "caches": {}, "power_policy": None,
    }
    if sys.platform == "win32":
        import winreg

        def res_value(key, value):
            try:
                with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key) as handle:
                    return winreg.QueryValueEx(handle, value)[0]
            except OSError:
                return None

        hardware = r"HARDWARE\DESCRIPTION\System"
        details["cpu"] = res_value(hardware + r"\CentralProcessor\0", "ProcessorNameString")
        details["model"] = res_value(hardware + r"\BIOS", "SystemProductName")
        details["power_policy"] = command_output("powercfg", "/getactivescheme")
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        read_topology = kernel.GetLogicalProcessorInformationEx
        read_topology.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulong)]
        length = ctypes.c_ulong()
        read_topology(0xffff, None, ctypes.byref(length))  # RelationAll
        data = ctypes.create_string_buffer(length.value)
        if length.value and read_topology(0xffff, data, ctypes.byref(length)):
            offset = cores = 0
            caches = {}
            while offset < length.value:
                relation, size = struct.unpack_from("<II", data, offset)
                if relation == 0:  # RelationProcessorCore
                    cores += 1
                elif relation == 2:  # RelationCache
                    level, capacity, kind = struct.unpack_from("<B3xII", data, offset + 8)
                    key = f"L{level} {['unified', 'instruction', 'data', 'trace'][kind]}"
                    caches.setdefault(key, set()).add(f"{capacity // 1024} KiB")
                offset += size
            details["physical_cores"] = cores
            details["caches"] = {key: sorted(values) for key, values in sorted(caches.items())}
        memory = (ctypes.c_uint64 * 8)()
        memory[0] = ctypes.sizeof(memory)  # dwLength followed by dwMemoryLoad
        if kernel.GlobalMemoryStatusEx(ctypes.byref(memory)):
            details["ram_bytes"] = memory[1]
    elif sys.platform.startswith("linux"):
        cpuinfo = read_text("/proc/cpuinfo") or ""
        for line in cpuinfo.splitlines():
            if line.startswith("model name"):
                details["cpu"] = line.partition(":")[2].strip()
                break
        details["model"] = read_text("/sys/class/dmi/id/product_name")
        details["power_policy"] = read_text("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
        for index in sorted(Path("/sys/devices/system/cpu/cpu0/cache").glob("index*")):
            level, kind, capacity = (read_text(index / name) for name in ("level", "type", "size"))
            if all((level, kind, capacity)):
                details["caches"][f"L{level} {kind.lower()}"] = [capacity + "iB" if capacity.endswith("K") else capacity]
        meminfo = read_text("/proc/meminfo") or ""
        if meminfo.startswith("MemTotal:"):
            details["ram_bytes"] = int(meminfo.split()[1]) * 1024
        details["physical_cores"] = command_output("lscpu", "-p=core,socket")
        if details["physical_cores"]:
            details["physical_cores"] = len({line for line in details["physical_cores"].splitlines()
                                             if line and not line.startswith("#")})
    return details


def compile_program(compiler, name, sources, flags):
    binary = BUILD / (name + (".exe" if os.name == "nt" else ""))
    subprocess.run([compiler, *flags, *sources, "-o", str(binary)], cwd=ROOT, check=True)
    return binary


def smoke_tests(compiler):
    sources = [(str(path.relative_to(ROOT)),) for path in sorted((ROOT / "tests").glob("*_smoke.cpp"))]
    sources += [("src/structB/test_structB.cpp",), ("oracle/oracle_smoke_test.cpp", "oracle/oracle.cpp")]
    flags = ["-std=c++17", "-O2", "-Wall", "-Wextra", "-pedantic"]
    for files in sources:
        binary = compile_program(compiler, Path(files[0]).stem, files, flags)
        subprocess.run([str(binary)], cwd=ROOT, check=True)
    app = compile_program(compiler, "sensor_demo", ["app/sensor_demo.cpp"], flags)
    demo = "3\n4 9 9\n6 15 21\n5\nQ 1 3\nR 2 2\nQ 1 3\nC 1 10\nQ 1 3\n"
    executed = subprocess.run([str(app)], input=demo, capture_output=True, text=True, check=True)
    if executed.stdout != "9 2 3\n9 1 3\n9 1 1\n":
        raise RuntimeError(f"Unexpected sensor demo output: {executed.stdout!r}")
    print("Smoke tests and sensor demo passed.", flush=True)


def make_metadata(compiler, profile, seed, runs, case, start, seconds):
    source_paths = sorted(set(SOURCES) | {
        str(path.relative_to(ROOT)).replace(os.sep, "/")
        for directory in ("bench", "src", "oracle", "include", "adversary")
        for path in (ROOT / directory).rglob("*.hpp")
    } | {"bench/run.py", "results/plot.py"})
    return {
        "started_utc": start,
        "elapsed_wall_seconds": seconds,
        "profile": profile,
        "case_filter": case,
        "seed": seed,
        "warmups": 1,
        "runs": runs,
        "oracle_scan_budget": 2_000_000 if profile == "quick" else 100_000_000,
        "compiler": Path(compiler).name,
        "compiler_version": command_output(compiler, "--version"),
        "compiler_flags": FLAGS,
        "host": host_details(),
        "python": platform.python_version(),
        "threads": 1,
        "affinity": "OS scheduled",
        "git_head_at_run": command_output("git", "rev-parse", "HEAD"),
        "sources_sha256": {
            file: hashlib.sha256((ROOT / file).read_bytes()).hexdigest()
            for file in source_paths
        },
    }


def publish(stage, destination, metadata):
    raw = destination / "raw"
    raw.mkdir(parents=True, exist_ok=True)
    runs = {}
    with (stage / "runs.csv").open(encoding="utf-8", newline="") as file:
        reader = csv.DictReader(file)
        for row in reader:
            family = row["case"].split("-", 1)[0]
            runs.setdefault(family, []).append(row)
    for family, rows in runs.items():
        with (raw / f"{family}.csv").open("w", encoding="utf-8", newline="") as file:
            writer = csv.DictWriter(file, reader.fieldnames, lineterminator="\n")
            writer.writeheader()
            writer.writerows(rows)
    for filename in ("checks.csv", "update_work.csv", "index_probe.csv", "targets.csv"):
        shutil.copyfile(stage / filename, raw / filename)
    (destination / "environment.json").write_text(
        json.dumps(metadata, indent=2) + "\n", encoding="utf-8",
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=["quick", "full"], default="full")
    parser.add_argument("--runs", type=int, default=RUNS)
    parser.add_argument("--seed", type=int, default=SEED)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--out", type=Path)
    parser.add_argument("--case", help="run one experiment ID, e.g. E2-mix-50")
    parser.add_argument("--all", action="store_true", help="also run smoke tests and the sensor demo")
    parser.add_argument("--build-only", action="store_true", help="build the harness and smoke-test the application")
    args = parser.parse_args()
    if not (0 <= args.seed <= 2**63 - 1 and 5 <= args.runs <= 100):
        parser.error("expected seed in [0, 2^63-1] and 5 to 100 timed runs")
    if args.case is None and not args.build_only:
        import matplotlib  # Check the plot dependency before starting the full run.

    BUILD.mkdir(parents=True, exist_ok=True)
    if args.all or args.build_only:
        smoke_tests(args.cxx)
    binary = compile_program(args.cxx, "benchmark", SOURCES, FLAGS)
    if args.build_only:
        return
    destination = (args.out or BUILD / (args.case or args.profile)).resolve()
    if args.case and destination == ROOT / "results":
        parser.error("use a separate output directory for a single experiment")
    stage = Path(tempfile.mkdtemp(prefix="run-", dir=BUILD))
    cmd = [str(binary), "--profile", args.profile, "--seed", str(args.seed),
           "--runs", str(args.runs), "--out", str(stage)]
    if args.case:
        cmd += ["--case", args.case]
    started = datetime.now(timezone.utc).isoformat(timespec="seconds")
    start = time.monotonic()
    try:
        subprocess.run(cmd, cwd=ROOT, check=True)
    except subprocess.CalledProcessError:
        print(f"Failure logs and any replay traces are in {stage}", file=sys.stderr)
        raise
    seconds = round(time.monotonic() - start, 2)
    publish(stage, destination, make_metadata(args.cxx, args.profile, args.seed, args.runs, args.case, started, seconds))
    if not args.case:
        subprocess.run([sys.executable, str(ROOT / "results/plot.py"), str(destination)],
                       cwd=ROOT, check=True)
    print(f"Results saved in {destination} ({seconds:.1f} s for the C++ run)")


if __name__ == "__main__":
    main()
