#!/usr/bin/env python3
"""
Linear fit on (R, total_time) and (R, total_misses) to separate
per-BFS cost from one-time setup cost.

  T(R) = T_setup + R * T_bfs

  - slope = per-BFS cost
  - y-intercept = one-time setup (graph loading + any conversion)
  - intercept_csr - intercept_pointer = cost of convert_to_csr alone
"""
import os
import re
import glob
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
MIN_R_FOR_TIME = 2   # drop R=1 from timing fit (cold-cache outlier)
MIN_R_FOR_CACHE = 1  # cache fits are linear from R=1


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLOTS = f"{ROOT}/plots"
RESULTS = f"{ROOT}/results/repeat"
os.makedirs(PLOTS, exist_ok=True)

COLOR = {"pointer": "#d62728", "csr": "#1f77b4"}


# ---------- TIMING ----------

def load_timing():
    df = pd.read_csv(f"{RESULTS}/time_raw.csv")
    # min across 5 trials per (R, impl) -- standard noise reduction
    agg = df.groupby(["R", "impl"], as_index=False)["time_ms"].min()
    return agg


def fit_and_report(x, y, label, min_R=2):
    """Linear fit y = m*x + c, optionally dropping points with x < min_R."""
    mask = x >= min_R
    if mask.sum() < 2:
        mask = np.ones_like(x, dtype=bool)  # fallback: use all
    m, c = np.polyfit(x[mask], y[mask], 1)
    print(f"  {label:8s}: per-unit = {m:.4f}, intercept = {c:.4f}  "
          f"({mask.sum()} points used)")
    return m, c

def plot_timing(df):
    fig, ax = plt.subplots(figsize=(7, 5))

    fits = {}
    for impl, sub in df.groupby("impl"):
        sub = sub.sort_values("R")
        x = sub["R"].to_numpy(dtype=float)
        y = sub["time_ms"].to_numpy(dtype=float)
        m, c = fit_and_report(x, y, impl, min_R=2)   # <-- changed
        fits[impl] = (m, c)

        ax.plot(x, y, "o", color=COLOR[impl], ms=8, label=f"{impl} (measured)")
        x_line = np.linspace(0, x.max() * 1.05, 100)
        ax.plot(x_line, m * x_line + c, "--", color=COLOR[impl], lw=1.5,
                label=f"{impl} fit: t = {m:.3f}·R + {c:.2f}")

    # Annotations: y-intercepts
    ax.axvline(0, color="gray", lw=0.5)
    for impl, (m, c) in fits.items():
        ax.annotate(f"{impl} setup\n= {c:.2f} ms",
                    xy=(0, c), xytext=(8, c + (5 if impl == "pointer" else -5)),
                    fontsize=9, color=COLOR[impl],
                    arrowprops=dict(arrowstyle="->", color=COLOR[impl], lw=0.7))

    ax.set_xlabel("Repeat count R (number of BFS runs)")
    ax.set_ylabel("Total time (ms)")
    ax.set_title("Total runtime vs repeat count (R=1 excluded from fit due to cold cache)\n"
                 "slope = per-BFS time, intercept = setup time")
    ax.legend(loc="upper left", fontsize=9)
    ax.grid(True, alpha=0.3)
    fig.savefig(f"{PLOTS}/09_repeat_time.png", dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {PLOTS}/09_repeat_time.png")
    return fits

# ---------- CACHE MISSES ----------

def parse_cachegrind(path):
    t = open(path).read()
    def grab(label):
        m = re.search(rf"{label}\s*:\s*([\d,]+)", t)
        return int(m.group(1).replace(",", "")) if m else None
    return {
        "D1_misses": grab("D1  misses"),
        "LLd_misses": grab("LLd misses"),
    }


def load_misses():
    rows = []
    for f in sorted(glob.glob(f"{RESULTS}/cache/r*_*.txt")):
        name = os.path.basename(f).replace(".txt", "")
        # name like "r5_csr"
        m = re.match(r"r(\d+)_(\w+)", name)
        if not m:
            continue
        R, impl = int(m.group(1)), m.group(2)
        rows.append({"R": R, "impl": impl, **parse_cachegrind(f)})
    return pd.DataFrame(rows)


def plot_misses(df, metric, title, fname):
    fig, ax = plt.subplots(figsize=(7, 5))
    fits = {}
    for impl, sub in df.groupby("impl"):
        sub = sub.sort_values("R")
        x = sub["R"].to_numpy(dtype=float)
        y = sub[metric].to_numpy(dtype=float)
        m, c = fit_and_report(x, y, f"{impl} {metric}")
        fits[impl] = (m, c)

        ax.plot(x, y / 1e6, "o", color=COLOR[impl], ms=8, label=f"{impl} (measured)")
        x_line = np.linspace(0, x.max() * 1.05, 100)
        ax.plot(x_line, (m * x_line + c) / 1e6, "--", color=COLOR[impl], lw=1.5,
                label=f"{impl} fit: {m/1e6:.3f}·R + {c/1e6:.2f} (M)")

    ax.axvline(0, color="gray", lw=0.5)
    ax.set_xlabel("Repeat count R")
    ax.set_ylabel(f"{metric} (millions)")
    ax.set_title(title)
    ax.legend(loc="upper left", fontsize=9)
    ax.grid(True, alpha=0.3)
    fig.savefig(f"{PLOTS}/{fname}", dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {PLOTS}/{fname}")
    return fits


# ---------- DERIVED NUMBERS ----------

def report_breakeven(time_fits):
    """Compute conversion overhead and break-even point."""
    m_p, c_p = time_fits["pointer"]
    m_c, c_c = time_fits["csr"]

    print("\n=== Breakdown (timing) ===")
    print(f"  pointer:  per-BFS = {m_p:.3f} ms,  setup (graph load) = {c_p:.3f} ms")
    print(f"  csr:      per-BFS = {m_c:.3f} ms,  setup (load + convert) = {c_c:.3f} ms")
    conv = c_c - c_p
    save_per_bfs = m_p - m_c
    print(f"\n  convert_to_csr cost  ≈ {conv:.3f} ms")
    print(f"  saving per BFS       ≈ {save_per_bfs:.3f} ms")
    if save_per_bfs > 0:
        breakeven = conv / save_per_bfs
        print(f"  CSR pays off after   ≈ {breakeven:.1f} BFS runs from the same graph")
    else:
        print("  CSR is slower per BFS too -- never pays off")


def report_miss_breakdown(d1_fits, lld_fits):
    print("\n=== Breakdown (cache misses, per BFS = slope) ===")
    print("  D1 misses:")
    for impl in ("pointer", "csr"):
        m, c = d1_fits[impl]
        print(f"    {impl:8s} per-BFS = {m/1e6:6.2f}M  setup = {c/1e6:6.2f}M")
    print("  LLd misses:")
    for impl in ("pointer", "csr"):
        m, c = lld_fits[impl]
        print(f"    {impl:8s} per-BFS = {m/1e6:6.2f}M  setup = {c/1e6:6.2f}M")


# ---------- MAIN ----------

def main():
    print("=== Timing fits ===")
    t_df = load_timing()
    time_fits = plot_timing(t_df)

    print("\n=== Cachegrind miss fits ===")
    m_df = load_misses()
    if m_df.empty:
        print("  no cache data found - skipping")
        return
    d1_fits  = plot_misses(m_df, "D1_misses",
                            "D1 misses vs repeat count", "10_repeat_d1.png")
    lld_fits = plot_misses(m_df, "LLd_misses",
                            "LLd misses vs repeat count", "11_repeat_lld.png")

    report_breakeven(time_fits)
    report_miss_breakdown(d1_fits, lld_fits)


if __name__ == "__main__":
    main()