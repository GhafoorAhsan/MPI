#!/bin/bash
# Extended cluster benchmark: strong scaling, random passwords, check_interval sweep at scale.
# Requires: hostfile62 (62 unique nodes), files/test4.enc/.sha512 ("ZZZZ"), run from the cluster.

OUTFILE="cluster_results.csv"
echo "experiment,length,password,method,check_interval,np,elapsed_seconds" > "$OUTFILE"

run() {
  # run <experiment> <length> <password_label> <np> <method> <check_interval|NA> <extra_args...>
  local EXP=$1 LEN=$2 PWLABEL=$3 NP=$4 METHOD=$5 CI=$6
  shift 6
  head -n "$NP" hostfile62 > /tmp/hf_tmp
  LINE=$(mpirun -np "$NP" --hostfile /tmp/hf_tmp ./"$METHOD" "$@" | grep "Elapsed time")
  TIME=$(echo "$LINE" | awk '{print $3}')
  echo "$EXP,$LEN,$PWLABEL,$METHOD,$CI,$NP,$TIME" >> "$OUTFILE"
  echo "[$EXP] len=$LEN pw=$PWLABEL np=$NP $METHOD ci=$CI -> ${TIME:-FAILED}"
}

# --- 1. Strong scaling: L=4, worst-case "ZZZZ", np = 1,2,4,8,16,32,62 ---
echo "1" > /tmp/hf1
run strong 4 ZZZZ 1 main_seq NA ./files/test4
for NP in 1 2 4 8 16 32 62; do
  run strong 4 ZZZZ "$NP" mainMPI NA ./files/test4
  run strong 4 ZZZZ "$NP" mainMPI_opt 1000 ./files/test4 1000
done

# --- 2. Random (non-extreme) length-4 passwords, np=16 ---
for PW in Kx9F 3mQa Tp7Z; do
  FILE="./files/rand_${PW}"
  if [ ! -f "${FILE}.enc" ]; then
    echo 'Hello World!' > "$FILE"
    ./encrypt "$FILE" "$PW"
  fi
  run random 4 "$PW" 16 mainMPI NA "$FILE"
  run random 4 "$PW" 16 mainMPI_opt 1000 "$FILE" 1000
done

# --- 3. check_interval sweep at np=62, L=4 worst-case ---
for CI in 10 25 50 100 500 1000 5000 10000; do
  run ci_sweep 4 ZZZZ 62 mainMPI_opt "$CI" ./files/test4 "$CI"
done

echo "Done. Results in $OUTFILE"
