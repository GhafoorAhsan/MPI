#!/bin/bash
# Benchmark sweep: password length (1-4) x check_interval (50,100,200,500,1000) + v2 baseline.
# Requires: ./files/test1 .. ./files/test4 already encrypted with passwords "Z","ZZ","ZZZ","ZZZZ".

NP=4
OUTFILE="benchmark_results.csv"
echo "length,method,check_interval,np,elapsed_seconds" > "$OUTFILE"

for L in 1 2 3 4; do
  FILE="./files/test${L}"

  echo "== length $L : v2 baseline =="
  LINE=$(mpirun -np $NP ./mainMPI "$FILE" | grep "Elapsed time")
  TIME=$(echo "$LINE" | awk '{print $3}')
  echo "$L,v2,NA,$NP,$TIME" >> "$OUTFILE"

  for CI in 50 100 200 500 1000; do
    echo "== length $L : v3 check_interval=$CI =="
    LINE=$(mpirun -np $NP ./mainMPI_opt "$FILE" $CI | grep "Elapsed time")
    TIME=$(echo "$LINE" | awk '{print $3}')
    echo "$L,v3,$CI,$NP,$TIME" >> "$OUTFILE"
  done
done

echo "Done. Results written to $OUTFILE"
