"""Summarize every timed run and regenerate the report figures from the raw CSVs."""

import argparse
from collections import defaultdict
import csv
from pathlib import Path
from statistics import mean, stdev

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import StrMethodFormatter

VARIANTS = {
    "SEG-MAX-COUNT": ("Segment tree", "#167984", "o"),
    "DC-MAX": ("Two-tree Fenwick", "#bc5025", "s"),
    "BIT-PREFIX-MAX": ("Prefix Fenwick", "#6b4da1", "D"),
    "ARRAY": ("Array oracle", "#42566d", "^"),
    "SEG-GCD": ("GCD segment tree", "#167984", "o"),
}
METRICS = ("node_reads", "node_writes", "combines", "recomputations", "gcd_steps")
GROUP_FIELDS = ("case", "mode", "variant", "query", "n", "operations", "width", "value_limit", "seed",
                "queries", "increases", "decreases", "unchanged")
SUMMARY_FIELDS = ("runs", "mean_ms", "sd_ms", "min_ms", "max_ms", "mean_ns_per_op", "sd_ns_per_op",
                  "mean_build_ms", "sd_build_ms", "min_build_ms", "max_build_ms", *METRICS)


def read_csv(path):
    with path.open(encoding="utf-8", newline="") as file:
        return list(csv.DictReader(file))


def summarize(results):
    groups = defaultdict(list)
    for path in sorted((results / "raw").glob("E[0-9]*.csv")):
        for run in read_csv(path):
            groups[tuple(run[field] for field in GROUP_FIELDS)].append(run)
    if not groups:
        raise ValueError(f"No experiment CSVs found in {results / 'raw'}")

    summaries = {}
    for key, runs in sorted(groups.items()):
        row = dict(zip(GROUP_FIELDS, key))
        if len(runs) < 5 or len({r["run"] for r in runs}) != len(runs):
            raise ValueError(f"Expected five or more distinct trials for {key[:3]}")
        metrics = (*METRICS, "checksum")
        if any(len({run[field] for run in runs}) != 1 for field in metrics):
            raise ValueError(f"Work counters or checksums vary across trials for {key[:3]}")

        elapsed = [int(run["elapsed_ns"]) / 1e6 for run in runs]
        build = [int(run["build_ns"]) / 1e6 for run in runs]
        factor = 1e6 / int(row["operations"])
        row.update(runs=len(runs), mean_ms=mean(elapsed), sd_ms=stdev(elapsed),
                   min_ms=min(elapsed), max_ms=max(elapsed),
                   mean_ns_per_op=mean(elapsed) * factor, sd_ns_per_op=stdev(elapsed) * factor,
                   mean_build_ms=mean(build), sd_build_ms=stdev(build),
                   min_build_ms=min(build), max_build_ms=max(build))
        row.update({field: int(runs[0][field]) for field in METRICS})
        if key[:3] in summaries:
            raise ValueError(f"Multiple trace configurations found for {key[:3]}")
        summaries[key[:3]] = row

    with (results / "summary.csv").open("w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, [*GROUP_FIELDS, *SUMMARY_FIELDS], lineterminator="\n")
        writer.writeheader()
        writer.writerows(summaries.values())
    return summaries


def select(data, case, variant, matched=False):
    if matched and (case, "matched", variant) in data:
        return data[case, "matched", variant]
    return data[case, "full", variant]


def timing_series(ax, x, rows, variant, spread="sd"):
    label, color, marker = VARIANTS[variant]
    averages = [r["mean_ns_per_op"] for r in rows]
    errors = [r["sd_ns_per_op"] for r in rows]
    if spread == "range":
        errors = [
            [a - r["min_ms"] * 1e6 / int(r["operations"]) for a, r in zip(averages, rows)],
            [r["max_ms"] * 1e6 / int(r["operations"]) - a for a, r in zip(averages, rows)],
        ]
    ax.errorbar(x, averages, yerr=errors, label=label, color=color, marker=marker, capsize=3)


def work_series(ax, x, rows, variant, metric):
    label, color, marker = VARIANTS[variant]
    ax.plot(x, [r[metric] / int(r["operations"]) for r in rows], label=label, color=color, marker=marker)


def save(fig, directory, name):
    fig.savefig(directory / f"{name}.png", dpi=155)
    plt.close(fig)


def widths(data, directory):
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), layout="constrained")
    for ax, prefix, title, variants in (
        (axes[0], "E1-width-", "Range maximum", ("SEG-MAX-COUNT", "DC-MAX", "ARRAY")),
        (axes[1], "E6-prefix-", "Prefix maximum", ("SEG-MAX-COUNT", "DC-MAX", "BIT-PREFIX-MAX", "ARRAY")),
    ):
        sizes = sorted({int(case[len(prefix):]) for case, _, _ in data
                        if case.startswith(prefix) and case[len(prefix):].isdigit()})
        for variant in variants:
            timing_series(ax, sizes, [select(data, f"{prefix}{w}", variant, matched=True) for w in sizes],
                          variant, spread="range")
        ax.set(xscale="log", yscale="log", xlabel="Inclusive width (elements)", ylabel="Time (ns/query)", title=title)
        ax.legend(loc="upper left")
    save(fig, directory, "width")


def mixes(data, directory):
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), layout="constrained")
    shares = [0, 10, 50, 90]
    for ax, cases, title, variants in (
        (axes[0], [f"E2-mix-{u}" for u in shares], "Range maximum",
         ("SEG-MAX-COUNT", "DC-MAX", "ARRAY")),
        (axes[1], ["E6-prefix-64", *[f"E6-prefix-mix-{u}" for u in shares[1:]]], "Prefix maximum",
         ("SEG-MAX-COUNT", "DC-MAX", "BIT-PREFIX-MAX", "ARRAY")),
    ):
        for variant in variants:
            timing_series(ax, shares, [select(data, case, variant) for case in cases], variant)
        ax.set(xticks=shares, yscale="log", xlabel="Assignments (% of operations)",
               ylabel="Time (ns/operation)", title=title)
        ax.legend(loc="upper left")
    save(fig, directory, "update_mix")


def decreases(data, directory):
    fig, ax = plt.subplots(figsize=(7.8, 4.2), layout="constrained")
    cases = ["E3-increase", *(f"E9-decrease-share-{d}" for d in (25, 50, 75)), "E4-decrease"]
    for variant in ("SEG-MAX-COUNT", "DC-MAX"):
        timing_series(ax, [0, 25, 50, 75, 100], [select(data, case, variant) for case in cases], variant)
        hot = select(data, "E5-hot-assign", variant)
        label, color, _ = VARIANTS[variant]
        ax.errorbar(50, hot["mean_ns_per_op"], yerr=hot["sd_ns_per_op"], label=f"{label}, hot positions",
                    marker="X", color=color, linestyle="none", capsize=3)
    ax.set(xticks=[0, 25, 50, 75, 100], xlabel="Decreases (% of assignments)", ylabel="Time (ns/assignment)")
    ax.legend()
    save(fig, directory, "decrease_share")


def scaling(data, directory):
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2), layout="constrained")
    prefix = "E10-scale-query-"
    sizes = sorted({int(case[len(prefix):]) for case, _, _ in data if case.startswith(prefix)})
    heights = [(n - 1).bit_length() for n in sizes]
    for ax, mode, ylabel, metric in (
        (axes[0], "query", "Node reads/query", "node_reads"),
        (axes[1], "assign", "Recomputations/assignment", "recomputations"),
    ):
        for variant in ("SEG-MAX-COUNT", "DC-MAX"):
            work_series(ax, sizes, [select(data, f"E10-scale-{mode}-{n}", variant) for n in sizes], variant, metric)
        if mode == "query":
            ax.plot(sizes, [2 * h + 1 for h in heights], color="#555555", linestyle="--", label="Bound 2h + 1")
        else:
            ax.plot(sizes, heights, color="#333333", linestyle="--", label="Segment tree prediction h")
        ax.set(xscale="log", xlabel="Array size (elements)", ylabel=ylabel)
        ax.legend()
    save(fig, directory, "predicted_work")


def index_profile(results, directory):
    rows = read_csv(results / "raw" / "index_probe.csv")
    n = max(int(r["n"]) for r in rows)
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.1), layout="constrained")
    for ax, metric, label in (
        (axes[0], "recomputations", "Recomputed cells/assignment"),
        (axes[1], "node_reads", "Node reads/assignment"),
    ):
        for direction, color, marker, desc in (("D", "#bc5025", "o", "Decrease"),
                                               ("I", "#167984", "x", "Increase")):
            buckets = defaultdict(list)
            for r in rows:
                if r["direction"] == direction:
                    buckets[(int(r["index"]) - 1) * 20 // n].append(int(r[metric]))
            x = [(b + 0.5) * n / 20 for b in sorted(buckets)]
            averages = [mean(buckets[b]) for b in sorted(buckets)]
            lows = [a - min(buckets[b]) for b, a in zip(sorted(buckets), averages)]
            highs = [max(buckets[b]) - a for b, a in zip(sorted(buckets), averages)]
            ax.errorbar(x, averages, yerr=[lows, highs], color=color, marker=marker, capsize=2,
                        linestyle="none", label=desc)
        ax.set(xlabel="Assignment position (20 index bins)", ylabel=label)
        ax.xaxis.set_major_formatter(StrMethodFormatter("{x:,.0f}"))
        ax.legend()
    save(fig, directory, "update_index")


def gcd_profiles(data, directory):
    fig, axes = plt.subplots(1, 2, figsize=(10.7, 4.1), layout="constrained")
    cases = ("E8-small", "E8-large", "E8-fibonacci")
    variants = ("SEG-GCD", "ARRAY")
    for i, variant in enumerate(variants):
        rows = [select(data, case, variant) for case in cases]
        label, color, marker = VARIANTS[variant]
        x = [j + (i - 0.5) * 0.18 for j in range(len(cases))]
        axes[0].errorbar(x, [r["mean_ns_per_op"] for r in rows],
                         yerr=[r["sd_ns_per_op"] for r in rows], label=label, color=color,
                         marker=marker, linestyle="none", capsize=4)
        axes[1].bar(x, [r["gcd_steps"] / int(r["operations"]) for r in rows],
                    width=0.17, label=label, color=color)
    for ax, ylabel in zip(axes, ("Time (ns/query)", "Euclid iterations/query")):
        ax.set(xticks=range(len(cases)), ylabel=ylabel)
        ax.set_xticklabels(["Multiples of 12\nw = 64", "Up to 10⁹\nw = 64", "Fibonacci pair\nw = 2"])
        ax.yaxis.labelpad = 8
        ax.legend()
    save(fig, directory, "gcd")


def prefix_rescans(data, directory):
    fig, axes = plt.subplots(1, 2, figsize=(10.5, 4.1), layout="constrained")
    cases = ("E6-control", "E6-rescan")
    for i, variant in enumerate(("SEG-MAX-COUNT", "DC-MAX", "BIT-PREFIX-MAX", "ARRAY")):
        label, color, marker = VARIANTS[variant]
        rows = [select(data, case, variant, matched=True) for case in cases]
        x = [j + (i - 1.5) * 0.15 for j in range(len(cases))]
        axes[0].errorbar(x, [r["mean_ns_per_op"] for r in rows],
                         yerr=[r["sd_ns_per_op"] for r in rows], label=label, color=color,
                         marker=marker, linestyle="none", capsize=3)
        axes[1].bar(x, [r["node_reads"] / int(r["operations"]) for r in rows],
                    width=0.14, label=label, color=color)
    for ax, ylabel in zip(axes, ("Time (ns/operation)", "Node reads/operation")):
        ax.set(xticks=range(len(cases)), yscale="log", ylabel=ylabel)
        ax.set_xticklabels(["Increase-only control", "Repeated max decreases"])
    axes[0].legend(loc="center")
    save(fig, directory, "prefix_rescan")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results", type=Path, nargs="?", default=Path(__file__).resolve().parent)
    args = parser.parse_args()
    plt.rcParams.update({
        "font.family": "DejaVu Sans", "font.size": 10, "axes.spines.top": False,
        "axes.spines.right": False, "axes.grid": True, "axes.axisbelow": True,
        "grid.alpha": 0.23, "legend.frameon": False,
    })
    directory = args.results / "figures"
    directory.mkdir(parents=True, exist_ok=True)
    data = summarize(args.results)
    for plot in (widths, mixes, decreases, scaling, gcd_profiles, prefix_rescans):
        plot(data, directory)
    index_profile(args.results, directory)


if __name__ == "__main__":
    main()
