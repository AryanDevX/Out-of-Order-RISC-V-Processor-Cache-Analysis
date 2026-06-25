#!/usr/bin/env python3
"""Generate all plots for the report."""
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLOTS = f"{ROOT}/plots"
os.makedirs(PLOTS, exist_ok=True)

COLOR = {"pointer": "#d62728", "csr": "#1f77b4"}  # red, blue
plt.rcParams.update({"font.size": 11, "axes.grid": True, "grid.alpha": 0.3})


def save(fig, name):
    path = f"{PLOTS}/{name}.png"
    fig.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  {path}")


def load():
    rt = pd.read_csv(f"{ROOT}/results/runtime.csv")
    # Normalize avg_deg (mixed types in star row)
    cache = pd.read_csv(f"{ROOT}/results/cache.csv")
    return rt, cache


def plot_runtime_vs_n(rt):
    size = rt[rt["groups"].str.contains("size")].sort_values(["n", "impl"])
    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    for impl, sub in size.groupby("impl"):
        ax.loglog(sub["n"], sub["min_ms"], "o-", lw=2, ms=7,
                  color=COLOR[impl], label=f"{impl} BFS")
    ax.set_xlabel("Vertices N (log scale)")
    ax.set_ylabel("BFS time (ms, log scale)")
    ax.set_title("BFS runtime vs graph size (ER graphs, avg deg = 8)")
    ax.legend()
    save(fig, "01_runtime_vs_n")


def plot_speedup_vs_n(rt):
    size = rt[rt["groups"].str.contains("size")]
    p = size.pivot(index="n", columns="impl", values="min_ms").sort_index()
    p["speedup"] = p["pointer"] / p["csr"]
    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    ax.semilogx(p.index, p["speedup"], "o-", color="#2ca02c", lw=2, ms=7)
    ax.axhline(1.0, color="k", lw=0.7, ls="--", alpha=0.6)
    ax.set_xlabel("Vertices N (log scale)")
    ax.set_ylabel("Speedup (pointer_ms / csr_ms)")
    ax.set_title("CSR speedup over pointer, across graph sizes")
    save(fig, "02_speedup_vs_n")


def plot_speedup_by_structure(rt):
    struct = rt[rt["groups"].str.contains("structure")]
    p = struct.pivot(index="graph", columns="impl", values="min_ms")
    p["speedup"] = p["pointer"] / p["csr"]
    # Order for narrative: highest speedup to lowest
    p = p.sort_values("speedup", ascending=False)

    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    colors = ["#2ca02c" if s >= 1 else "#d62728" for s in p["speedup"]]
    ax.bar(p.index, p["speedup"], color=colors, edgecolor="black")
    ax.axhline(1.0, color="k", lw=0.7, ls="--", alpha=0.6)
    ax.set_ylabel("Speedup (pointer_ms / csr_ms)")
    ax.set_title("CSR speedup by graph structure (N ≈ 100K)\n"
                 "Green = CSR faster, red = pointer faster")
    for i, v in enumerate(p["speedup"]):
        ax.text(i, v + 0.05, f"{v:.2f}×", ha="center", fontsize=10)
    save(fig, "03_speedup_by_structure")


def plot_speedup_vs_degree(rt):
    deg = rt[rt["groups"].str.contains("degree")].copy()
    deg["avg_deg"] = pd.to_numeric(deg["avg_deg"], errors="coerce")
    deg = deg.dropna(subset=["avg_deg"])
    p = deg.pivot(index="avg_deg", columns="impl", values="min_ms").sort_index()
    p["speedup"] = p["pointer"] / p["csr"]

    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    ax.plot(p.index, p["speedup"], "o-", color="#2ca02c", lw=2, ms=8)
    ax.axhline(1.0, color="k", lw=0.7, ls="--", alpha=0.6)
    ax.set_xlabel("Average degree")
    ax.set_ylabel("Speedup (pointer_ms / csr_ms)")
    ax.set_title("CSR speedup vs average degree (ER graphs, N = 100K)")
    ax.set_xticks(p.index)
    for x, v in zip(p.index, p["speedup"]):
        ax.text(x, v + 0.08, f"{v:.2f}×", ha="center", fontsize=10)
    save(fig, "04_speedup_vs_degree")


def plot_baseline_miss_rates(cache):
    b = cache[cache["sweep"] == "baseline"].set_index("impl")
    metrics = ["D1_rate", "LLd_rate"]
    labels = ["D1 miss rate", "LLd miss rate"]
    x = np.arange(len(metrics))
    w = 0.35

    fig, ax = plt.subplots(figsize=(6.5, 4.2))
    ax.bar(x - w/2, [b.loc["pointer", m] for m in metrics], w,
           color=COLOR["pointer"], label="pointer", edgecolor="black")
    ax.bar(x + w/2, [b.loc["csr",     m] for m in metrics], w,
           color=COLOR["csr"],     label="csr",     edgecolor="black")
    ax.set_xticks(x)
    ax.set_xticklabels(labels)
    ax.set_ylabel("Miss rate (%)")
    ax.set_title("Cache miss rates: pointer vs CSR (ER N=100K, deg=8)")
    ax.legend()
    for i, m in enumerate(metrics):
        ax.text(i - w/2, b.loc["pointer", m] + 0.05,
                f"{b.loc['pointer', m]:.2f}%", ha="center", fontsize=9)
        ax.text(i + w/2, b.loc["csr", m] + 0.05,
                f"{b.loc['csr', m]:.2f}%",     ha="center", fontsize=9)
    save(fig, "05_baseline_miss_rates")


def plot_sweep(cache, sweep, xlabel, xscale, title, fname, transform=int):
    sub = cache[cache["sweep"] == sweep].copy()
    sub["value"] = sub["value"].apply(transform)
    sub = sub.sort_values(["value", "impl"])

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11, 4.2))
    for impl, s in sub.groupby("impl"):
        ax1.plot(s["value"], s["D1_rate"], "o-", lw=2, ms=7,
                 color=COLOR[impl], label=impl)
        ax2.plot(s["value"], s["LLd_rate"], "o-", lw=2, ms=7,
                 color=COLOR[impl], label=impl)
    for ax, ylab in zip([ax1, ax2], ["D1 miss rate (%)", "LLd miss rate (%)"]):
        ax.set_xlabel(xlabel)
        ax.set_ylabel(ylab)
        ax.set_xscale(xscale)
        ax.legend()
    fig.suptitle(title)
    fig.tight_layout()
    save(fig, fname)


def main():
    rt, cache = load()
    print("writing plots:")
    plot_runtime_vs_n(rt)
    plot_speedup_vs_n(rt)
    plot_speedup_by_structure(rt)
    plot_speedup_vs_degree(rt)
    plot_baseline_miss_rates(cache)
    plot_sweep(cache, "size",  "D1 size (bytes, log)", "log",
               "Miss rate vs D1 size (assoc=4, line=64)", "06_sweep_size")
    plot_sweep(cache, "assoc", "D1 associativity (log)", "log",
               "Miss rate vs D1 associativity (size=32K, line=64)", "07_sweep_assoc")
    plot_sweep(cache, "line",  "Cache line size (bytes, log)", "log",
               "Miss rate vs line size (size=32K, assoc=4)", "08_sweep_line")


if __name__ == "__main__":
    main()