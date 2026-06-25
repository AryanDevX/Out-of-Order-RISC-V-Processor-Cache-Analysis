#!/bin/bash
set -e

GRAPH=data/er_n100000_d8.txt
mkdir -p results/repeat results/repeat/cache

echo "=== Part 1: timing sweep ==="
echo "R,impl,trial,time_ms" > results/repeat/time_raw.csv

# Wider R range for the timing fit (quick to run natively)
for R in 1 2 5 10 20 50 100; do
  for impl in pointer csr; do
    for trial in 1 2 3 4 5; do
      out=$(./graph_bench --impl=$impl --graph=$GRAPH --source=0 --repeat=$R)
      t=$(echo "$out" | grep time_ms | cut -d= -f2)
      echo "$R,$impl,$trial,$t" >> results/repeat/time_raw.csv
    done
    echo "  done R=$R impl=$impl"
  done
done

echo ""
echo "=== Part 2: cachegrind sweep ==="
echo "(slow - about 1-2 min per run, 8 runs total)"

# Smaller R range because cachegrind is slow.
# Need at least 3 points for a line fit; use 4 for safety.
for R in 1 5 10 20; do
  for impl in pointer csr; do
    echo "  cachegrind R=$R impl=$impl"
    valgrind --tool=cachegrind --cache-sim=yes \
      --I1=32768,8,64 --D1=32768,4,64 --LL=8388608,16,64 \
      --cachegrind-out-file=results/repeat/cache/r${R}_${impl}.out \
      ./graph_bench --impl=$impl --graph=$GRAPH --source=0 --repeat=$R \
      2> results/repeat/cache/r${R}_${impl}.txt
  done
done

echo ""
echo "=== Done. Now run scripts/repeat_analyze.py ==="