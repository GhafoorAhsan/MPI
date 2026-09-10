#include <stdio.h>
#include <stdint.h>
#include "aes256.h"
#include "utils.h"
#include <math.h>
#include <time.h>
#include <mpi.h>

struct timespec start, end;

#define CHARSET_SIZE 62
#define CHECK_INTERVAL 1000 // Check every 1000 rounds 

// Character set used for passwords [0-9a-zA-Z]
char CHARSET[CHARSET_SIZE]={
    '0','1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a','b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A','B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

void index_to_password(uint64_t index, int L, char *pwd) {
    for (int i = 0; i < L; i++) {
        pwd[L-1-i] = CHARSET[index % CHARSET_SIZE];
        index /= CHARSET_SIZE;
    }
    pwd[L] = '\0'; // Null terminate for the string 
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv); // Init MPI environment 
    uint8_t key[AES_256_KEY_LENGTH]; // Stores the generate AES key
    uint8_t ciphertext[MAX_FILE_SIZE_B]; // Stores encrypted data
    uint8_t plaintext[MAX_FILE_SIZE_B]; // Stores decrypted data
    uint8_t plaintext_checksum[SHA512_DIGEST_LENGTH]; // Stores the original file sha512 sum
    uint8_t computed_checksum[SHA512_DIGEST_LENGTH]; // Stores the decrypted file sha512 sum

    char enc_path[300];
    char sha512_path[300];

    if (argc < 2) {
        printf("Usage: %s <file_base_path>\n", argv[0]);
        MPI_Finalize(); // Finalize the MPI Environment 
        return -1;
    } else {
        snprintf(enc_path, sizeof(enc_path), "%s.enc", argv[1]);
        snprintf(sha512_path, sizeof(sha512_path), "%s.sha512", argv[1]);
    }

    uint32_t ciphertext_length = file_load(enc_path, ciphertext); // Load encrypted file into memory
    file_load(sha512_path, plaintext_checksum); // Load the original file checksum

    int found = 0;
    int global_found = 0;
    int rank, size; 

    // Timing starts here 
    MPI_Barrier(MPI_COMM_WORLD); // Ensure all processes start timing at the same time 
    clock_gettime(CLOCK_MONOTONIC, &start);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (int L = 1; !global_found; L++) {

        uint64_t num_guess = (uint64_t)pow(62, L);
        uint64_t chunk_size = num_guess / size;
        uint64_t remainder = num_guess % size;
        // We calculate the start and end indices for each process, taking into account the remainder
        uint64_t partition_start = rank * chunk_size + (rank < remainder ? rank : remainder);
        // (rank+1) computes where the NEXT rank's chunk would start, which is exactly this rank's own exclusive end boundary
        uint64_t partition_end = (rank + 1) * chunk_size + ((rank + 1) < remainder ? (rank + 1) : remainder);

        char pwd [L+1];

        // Checking in every CHECK_INTERVAL guesses (instead of once per L) means MORE Allreduce calls, trading communication
        // overhead for less wasted work after an early find. num_checkpoints is derived from chunk_size (identical on every
        // rank), not this rank's own partition size, so every rank computes the same checkpoint count, using partition
        // size could differ by 1 due to the remainder and desync the Allreduce call counts.
        uint64_t num_checkpoints = chunk_size / CHECK_INTERVAL;
        uint64_t pos = partition_start; 

        // Must be global_found, not found: Allreduce is collective, so every rank must call it the same number of times.
        // If a rank stopped this loop as soon as its OWN found became true, it would make fewer Allreduce calls than ranks
        // still searching, and they'd hang waiting for it forever.
        for (uint64_t round = 0; round < num_checkpoints && !global_found; round++) { 
            uint64_t round_end = pos + CHECK_INTERVAL; 
            for (; pos < round_end && !found; pos++) {
                index_to_password(pos, L, pwd);
                pbkdf2(pwd, L, key);
                int32_t plaintext_length = decrypt(ciphertext, ciphertext_length, key, plaintext);

                if(plaintext_length >= 0) { 
                    sha512sum(plaintext,plaintext_length,computed_checksum);
                    if(sha512cmp(plaintext_checksum,computed_checksum) == 0) { 
                        found = 1;
                        plaintext[plaintext_length]='\0'; 
                        printf("Encrypted file contains: %s\n",plaintext);
                        printf("%s\n", pwd);
                    }
                }
            }
            MPI_Allreduce(&found, &global_found, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        }

        // This block (and its Allreduce) always runs when global_found is still false, even if this rank has 0 guesses
        // left, skipping it for ranks with nothing left would again desync the Allreduce call count across ranks.
        if (!global_found) {
            for (; pos < partition_end && !found; pos++) {
                index_to_password(pos, L, pwd);
                pbkdf2(pwd, L, key);
                int32_t plaintext_length = decrypt(ciphertext, ciphertext_length, key, plaintext);

                if(plaintext_length >= 0) { 
                    sha512sum(plaintext,plaintext_length,computed_checksum);
                    if(sha512cmp(plaintext_checksum,computed_checksum) == 0) { 
                        found = 1;
                        plaintext[plaintext_length]='\0'; 
                        printf("Encrypted file contains: %s\n",plaintext);
                        printf("%s\n", pwd);
                    }
                }
            }
            MPI_Allreduce(&found, &global_found, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        }

    }

    // end is stamped here, not inside the match block: capturing it there only measured how fast the ONE rank that
    // found the password did so locally, not when the whole program actually finished, other ranks could still be
    // searching for seconds afterward. Stamping it after the loop (a point every rank reaches at the same synchronized
    // moment, right after the same Allreduce call) measures the real total time.
    clock_gettime(CLOCK_MONOTONIC, &end);
    MPI_Finalize(); // Finalize the MPI Environment 

    if (found) {
        double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; // 1000000000
        printf("Elapsed time: %f seconds\n", elapsed_time);
    }
    
    return 0;
}
