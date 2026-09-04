#include <stdio.h>
#include <stdint.h>
#include "aes256.h"
#include "utils.h"
#include <math.h>
#include <time.h>

struct timespec start, end; 


#define CHARSET_SIZE 62

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

// Hardcoded password "abc". Template for inner loop, not the solution
int main(int argc, char *argv[]) {
  uint8_t key[AES_256_KEY_LENGTH]; // Stores the generate AES key

  // uint8_t ciphertext[100]; // Stores encrypted data, size this to MAX_FILE_SIZE_B for bigger test 
  uint8_t ciphertext[MAX_FILE_SIZE_B]; // Stores encrypted data, size this to MAX_FILE_SIZE_B for bigger test 

  // uint8_t plaintext[100]; // Stores decrypted data
  uint8_t plaintext[MAX_FILE_SIZE_B]; // Stores decrypted data
  
  uint8_t plaintext_checksum[SHA512_DIGEST_LENGTH]; // Stores the original file sha512 sum
  uint8_t computed_checksum[SHA512_DIGEST_LENGTH]; // Stores the decrypted file sha512 sum

  // char password[]="abc"; // Stores the user supplied password

  // pbkdf2(password, 3, key); // Generate the AES-256 key from the password

  uint32_t ciphertext_length = file_load("./files/myfile.enc", ciphertext); // Load encrypted file into memory
  // // Bug? Should be "./files/..."
  // // file_load("/files/myfile.sha512",plaintext_checksum); // Load the original file checksum

  file_load("./files/myfile.sha512",plaintext_checksum); // Load the original file checksum

  // int32_t plaintext_length=decrypt(ciphertext, ciphertext_length, key, plaintext); // Try to decrypt and retrieve the decripted file length
  
  // if(plaintext_length>=0){ // Ensure decryption succeeded
  //   sha512sum(plaintext,plaintext_length,computed_checksum); // Compute decrypted data sha512 sum

  //   // if(sha512cmp(plaintext_checksum,computed_checksum)){ // If sha512 match with the original file we succeeded
  //   //   plaintext[plaintext_length]='\0'; // Make plaintext data "printable"
  //   //   printf("Encrypted file contains: %s\n",plaintext); // Print decrypted data
  //   // }
  //   if(sha512cmp(plaintext_checksum,computed_checksum) == 0){ // If sha512 match with the original file we succeeded
  //     plaintext[plaintext_length]='\0'; // Make plaintext data "printable"
  //     printf("Encrypted file contains: %s\n",plaintext); // Print decrypted data
  //   }
  // }

  // char test[4];
  // index_to_password(62, 2, test);
  // printf("%s\n", test); // Should print 01C for index = 100, L = 3, pwd = test, Should print 010 for index = 62, L = 3, pwd = test, 10 for index = 62, L = 2, pwd = test

  int found = 0;

  // Timing starts here 
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (int L = 1; !found; L++) {
    uint64_t num_guess = (uint64_t)pow(62, L);
    char pwd [L+1];
    for (uint64_t i = 0; i < num_guess && !found; i++) {
      index_to_password(i, L, pwd);
      pbkdf2(pwd, L, key);
      int32_t plaintext_length = decrypt(ciphertext, ciphertext_length, key, plaintext);
      if(plaintext_length>=0){ 
        sha512sum(plaintext,plaintext_length,computed_checksum);
        if(sha512cmp(plaintext_checksum,computed_checksum) == 0){ 
          found = 1;
          clock_gettime(CLOCK_MONOTONIC, &end);
          plaintext[plaintext_length]='\0'; 
          printf("Encrypted file contains: %s\n",plaintext);
          printf("%s\n", pwd);
        }
      }
    }
  }
  double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; // 1000000000
  printf("Elapsed time: %f seconds\n", elapsed_time);
  return 0;
}
