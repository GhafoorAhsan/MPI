# Assignment 1

Executing the `make` command will build the code example and the tool for encrypting files.

- `main.c` contains the code example to decrypt a file, kept untouched as a reference
- `main_seq.c`is the finished sequential solution
- `mainMPI.c`is the finished MPI parallel solution
- `mainMPI_opt.c`is the MPI solution with the checkpointing optimization
- `encrypt.c` contains the encryption tool implementation, used to generate the test files
- `files/` stores the plaintext/encrypted/checksum files you are working with

**Important note:** If you encrypt your own files with external tools you must ensure that the proper salt and Initialization Vector are used.

## Building

```sh
make all # builds main, encrypt, main_seq, mainMPI, mainMPI_opt
make clean # removes all built binaries
```

On macOS, the `Makefile`picks up OpenSSL via `brew --prefix openssl@3`, so `brew install openssl@3` first
if you don't have it.
On the cluster, just `make`, the Linux branch links `-lcrypto -lm` directly.

## Running

All three solutions take a **base file path** and derive `<path>.enc`/ `<path>.sha512` from it
Example: `./files/test4` resolves to `./files/test4.enc` and `./files/test4.sha512``

**Sequential:**

```sh
./main_seq ./files/test4
```

**MPI parallel (v2):**

```sh
mpirun -np 4 ./mainMPI ./files/test4
```

**MPI parallel with checkpointing optimization (v3):**

Optional second argument sets `check_interval`(default `1000`):

```sh
mpirun -np 4 ./mainMPI_opt ./files/test4 1000
```

Each run prints the recovered plaintext, the password found, and `Èlapsed time: <seconds> seconds` once the search complettes.

**Generating your own test files:**

```sh
echo `some text` > ./files/mytest # use single quotes to avoid shell quoting issues
./encrypt ./files/mytest <password>
```

This produces `./files/mytest.enc` and `./files/mytest.sha512`.

## Cluster access

The Computer Science department has a cluster of nodes that can be used to run your solution. By copying your files on the front-end (ificluster.ifi.uit.no) you will be able to access them from all the other nodes (shared file system). Be mindful of your processes and remember to quit them properly, as resources are shared among all users. In order to login to the cluster from Linux, use:

`ssh <your_UiT_ID>@ificluster.ifi.uit.no`

Make sure that you are familiar with the welcome message from the cluster, helping you to use the cluster correctly.

## Run experiments

From your machine, copy your assignment folder on your homedir:

- Create a destination folder on the cluster:

`ssh <your_UiT_ID>@ificluster.ifi.uit.no 'mkdir <assignment_folder>'`

- Copy assignment folder to cluster:

`scp -r <assignment_folder>/* <your_UiT_ID>@ificluster.ifi.uit.no:<assignment_folder>`

Create a hostfile:

- Access the cluster: `ssh <your_UiT_ID>@ificluster.ifi.uit.no`
- Go into the assignment folder: `cd <assignment_folder>`
- List available nodes `/share/ifi/available-nodes.sh`
- Put the nodes you need in `hostfile`, in the same format as the one print by the previous script
- Compile your mpi version: `make mainMPI`
- Use the `mpirun` command to run the code on the cluster using the hostfile. You must ensure a fair distribution of the processes across the nodes. You can check it by listing the processes running on the nodes.

## Benchmarking

**Local benchmark (`benchmark.sh`):**

Requires `./files/test1` through `./files/test4`to already exist, encrypted with passwords `"Z"`, `"ZZ"`, `"ZZZ"`, `"ZZZZ"` respectively:

```sh
./encrypt ./files/test1 Z
./encrypt ./files/test2 ZZ
./encrypt ./files/test3 ZZZ
./encrypt ./files/test4 ZZZZ
```

Then run it:

```sh
./benchmark.sh
```

At a fixed `-np 4`, it sweeps password length `L=1..4`, running `mainMPI`once per length as the v2 baseline, then `mainMPI_opt`per length across `check_interval`
values `50, 100, 200, 500, 1000`. Results are qritten to `benchmark_results.cvs` with coloums `length, method, check_interval, np, elapsed_seconds`.

**Cluster benchmark (`cluster_benchmark.sh`):**

Run on the cluster, from inside the assignment folder. Requires `hostfile62` (62 unique nodes, see [Cluster access](#cluster-access)) and `./files/test4` encrypted with password `"ZZZZ"` :

Then run it:

```sh
./cluster_benchmark.sh
```

It runs three experiment group: strong scaling (`L=4`, worst-case `"ZZZZ"`, `np=1,2,4,8,16,32,62` for `main_seq`/`mainMPI`/`mainMPI_opt`), random non-extreme passwords (`np=16`, `mainMPI` vs `mainMPI_opt`), and a `check_interval` sweep (`np=62`, `L=4` worst-case, `check_interval=10,25,50,100,500,1000,5000,10000`). Each run uses the first `np` lines of `hostfile62` to ensure a fair, non-oversubscribed node spread. Results are written to `cluster_results.csv` with columns `experiment, length, password, method, check_interval, np, elapsed_seconds`.
