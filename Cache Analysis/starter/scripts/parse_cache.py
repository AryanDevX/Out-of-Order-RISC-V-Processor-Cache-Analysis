#!/usr/bin/env python3
"""
Parse all cachegrind summary files in results/cache/*.txt into one CSV.

Filename convention:
  baseline_{impl}.txt
  size_{bytes}_{impl}.txt
  assoc_{ways}_{impl}.txt
  line_{bytes}_{impl}.txt
"""
import re
import glob
import csv
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE_DIR = f"{ROOT}/results/cache"
OUT = f"{ROOT}/results/cache.csv"


def parse(path):
    t = open(path).read()

    def grab(label):
        # matches "D1  misses:   12,308,717"
        m = re.search(rf"{label}\s*:\s*([\d,]+)", t)
        return int(m.group(1).replace(",", "")) if m else None

    def grab_rate(label):
        # matches "D1  miss rate:           3.1%"
        m = re.search(rf"{label}\s*:\s*([\d.]+)\s*%", t)
        return float(m.group(1)) if m else None

    return dict(
        I_refs      = grab("I refs"),
        D_refs      = grab("D refs"),
        D1_misses   = grab("D1  misses"),
        LLd_misses  = grab("LLd misses"),
        D1_rate     = grab_rate("D1  miss rate"),
        LLd_rate    = grab_rate("LLd miss rate"),
    )


def classify(name):
    """Extract sweep name, parameter value, impl from filename."""
    stem = name.replace(".txt", "")
    parts = stem.split("_")
    # "baseline_pointer" -> sweep=baseline, value=default, impl=pointer
    if parts[0] == "baseline":
        return "baseline", "default", parts[1]
    # "size_32768_pointer" -> sweep=size, value=32768, impl=pointer
    return parts[0], parts[1], parts[2]


def main():
    rows = []
    files = sorted(glob.glob(f"{CACHE_DIR}/*.txt"))
    for f in files:
        name = os.path.basename(f)
        sweep, value, impl = classify(name)
        stats = parse(f)
        # Sanity check: if we got None for a critical field, the run failed.
        if stats["D1_rate"] is None:
            print(f"warn: could not parse {name} — likely a failed cachegrind run")
            continue
        rows.append(dict(
            sweep=sweep, value=value, impl=impl, file=name, **stats
        ))

    if not rows:
        print("no rows parsed — check results/cache/ contents")
        return

    with open(OUT, "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    print(f"wrote {OUT}  ({len(rows)} rows)")
    # Quick summary to stdout
    print("\n=== Baseline summary ===")
    for r in rows:
        if r["sweep"] == "baseline":
            print(f"  {r['impl']:7s}  D1={r['D1_rate']:.2f}%  LLd={r['LLd_rate']:.2f}%  "
                  f"D1_miss={r['D1_misses']:,}  LLd_miss={r['LLd_misses']:,}")


if __name__ == "__main__":
    main()