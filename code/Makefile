UNAME := $(shell uname -s)

ifeq ($(UNAME),Darwin)
	OPENSSL := $(shell brew --prefix openssl@3)
	CFLAGS  += -I$(OPENSSL)/include
	LDFLAGS += -L$(OPENSSL)/lib
endif

LDLIBS += -lcrypto

all: main encrypt mainMPI mainMPI_opt main_seq

main: main.c aes256.c utils.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

encrypt: encrypt.c aes256.c utils.c
	gcc $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

mainMPI: mainMPI.c aes256.c utils.c
	mpicc $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

mainMPI_opt: mainMPI_opt.c aes256.c utils.c
	mpicc $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

main_seq: main_seq.c aes256.c utils.c
	mpicc $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	@rm -f main encrypt mainMPI mainMPI_opt main_seq