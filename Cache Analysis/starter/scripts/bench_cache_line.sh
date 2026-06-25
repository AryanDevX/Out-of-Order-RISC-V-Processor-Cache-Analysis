#!/bin/bash
set -e
GRAPH=data/er_n100000_d8.txt
ARGS="--source=0 --repeat=10"

for ls in 32 64 128; do
  for impl in pointer csr; do
    echo "  [line_${ls} $impl]"
    valgrind --tool=cachegrind --cache-sim=yes \
      --I1=32768,8,$ls --D1=32768,4,$ls --LL=8388608,16,$ls \
      --cachegrind-out-file=results/cache/line_${ls}_${impl}.out \
      ./graph_bench --impl=$impl --graph=$GRAPH $ARGS \
      2> results/cache/line_${ls}_${impl}.txt
  done
done

# Clean up the failed line=16 file
rm -f results/cache/line_16_pointer.txt results/cache/line_16_pointer.out

echo "line sweep done"