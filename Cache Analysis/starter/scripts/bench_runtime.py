#!/usr/bin/env python3
"""
Runtime benchmarks: pointer-BFS vs CSR-BFS.

Methodology:
  - For each (graph, impl), run graph_bench with --repeat=REPEAT inside a
    single process (amortizes process startup + graph load; BFS runs on
    warm caches).
  - Repeat that TRIALS times as independent processes (captures across-
    process noise from scheduling, ASLR, etc.).
  - Report min (standard practice for timing benchmarks) plus median,
    mean, stdev for transparency.

Writes: results/runtime.csv
"""
import subprocess
import csv
import os
import statistics
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BENCH = f"{ROOT}/graph_bench"
DATA = f"{ROOT}/data"
RESULTS = f"{ROOT}/results"
os.makedirs(RESULTS, exist_ok=True)

TRIALS = 7       # independent invocations
REPEAT = 20      # inner --repeat per invocation


def one_run(impl, graph_path, repeat):
    r = subprocess.run(
        [BENCH, f"--impl={impl}", f"--graph={graph_path}",
         "--source=0", f"--repeat={repeat}"],
        capture_output=True, text=True, timeout=600,
    )
    if r.returncode != 0:
        print("graph_bench failed:", r.stderr, file=sys.stderr)
        sys.exit(1)
    lines = [x.strip() for x in r.stdout.strip().splitlines() if x.strip()]
    visited = int(lines[0].split("=")[1])
    total_ms = float(lines[1].split("=")[1])
    return total_ms / repeat, visited


def benchmark(impl, graph_path):
    times = []
    visited = None
    for _ in range(TRIALS):
        t, v = one_run(impl, graph_path, REPEAT)
        times.append(t)
        visited = v
    return {
        "min_ms": min(times),
        "median_ms": statistics.median(times),
        "mean_ms": statistics.mean(times),
        "stdev_ms": statistics.stdev(times) if len(times) > 1 else 0.0,
        "visited": visited,
    }


# (label, filename, groups it belongs to, n, avg_deg)
GRAPHS = [
    ("er_10k",      "er_n10000_d8.txt",   ["size"],                      10_000,    8),
    ("er_50k",      "er_n50000_d8.txt",   ["size"],                      50_000,    8),
    ("er_100k_d8",  "er_n100000_d8.txt",  ["size", "degree", "structure"], 100_000, 8),
    ("er_500k",     "er_n500000_d8.txt",  ["size"],                      500_000,   8),
    ("er_1m",       "er_n1000000_d8.txt", ["size"],                      1_000_000, 8),

    ("er_100k_d4",  "er_n100000_d4.txt",  ["degree"],                    100_000,   4),
    ("er_100k_d16", "er_n100000_d16.txt", ["degree"],                    100_000,   16),
    ("er_100k_d32", "er_n100000_d32.txt", ["degree"],                    100_000,   32),

    ("grid",        "grid_317x317.txt",   ["structure"],                 317 * 317, 4),
    ("chain",       "chain_n100000.txt",  ["structure"],                 100_000,   1),
    ("star",        "star_n100000.txt",   ["structure"],                 100_000,   "mixed"),
]


def main():
    rows = []
    for label, fname, groups, n, deg in GRAPHS:
        path = f"{DATA}/{fname}"
        if not os.path.exists(path):
            print(f"skip {label}: missing {path}", file=sys.stderr)
            continue
        print(f"\n=== {label} ({fname}) ===")
        for impl in ("pointer", "csr"):
            r = benchmark(impl, path)
            row = {
                "graph": label,
                "impl": impl,
                "groups": ",".join(groups),
                "n": n,
                "avg_deg": deg,
                **r,
            }
            rows.append(row)
            print(
                f"  {impl:7s} min={r['min_ms']:.3f}ms "
                f"median={r['median_ms']:.3f}ms "
                f"stdev={r['stdev_ms']:.4f}ms "
                f"visited={r['visited']}"
            )

    out = f"{RESULTS}/runtime.csv"
    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"\nwrote {out}  ({len(rows)} rows)")


if __name__ == "__main__":
    main()