#!/bin/bash
set -e
mkdir -p results/cache

GRAPH=data/er_n100000_d8.txt
ARGS="--source=0 --repeat=10"
COMMON="--tool=cachegrind --cache-sim=yes --I1=32768,8,64"

run_cg() {
  local tag=$1 d1_spec=$2 ll_spec=$3 impl=$4
  echo "  [$tag $impl]"
  valgrind $COMMON --D1=$d1_spec --LL=$ll_spec \
    --cachegrind-out-file=results/cache/${tag}_${impl}.out \
    ./graph_bench --impl=$impl --graph=$GRAPH $ARGS \
    2> results/cache/${tag}_${impl}.txt
}

echo "=== Baseline (default-ish: 32K/4/64 D1, 8M/16/64 LL) ==="
for impl in pointer csr; do
  run_cg baseline 32768,4,64 8388608,16,64 $impl
done

echo "=== D1 size sweep (assoc=4, line=64) ==="
for sz in 8192 16384 32768 65536 131072; do
  for impl in pointer csr; do
    run_cg size_${sz} ${sz},4,64 8388608,16,64 $impl
  done
done

echo "=== D1 associativity sweep (size=32K, line=64) ==="
for a in 1 2 4 8 16; do
  for impl in pointer csr; do
    run_cg assoc_${a} 32768,${a},64 8388608,16,64 $impl
  done
done

echo "=== D1 line-size sweep (size=32K, assoc=4) — LL line must match ==="
for ls in 16 32 64 128; do
  for impl in pointer csr; do
    # LL line size must equal L1 line size; regenerate I1 too
    valgrind --tool=cachegrind --cache-sim=yes \
      --I1=32768,8,$ls --D1=32768,4,$ls --LL=8388608,16,$ls \
      --cachegrind-out-file=results/cache/line_${ls}_${impl}.out \
      ./graph_bench --impl=$impl --graph=$GRAPH $ARGS \
      2> results/cache/line_${ls}_${impl}.txt
  done
done

echo "done — outputs in results/cache/"