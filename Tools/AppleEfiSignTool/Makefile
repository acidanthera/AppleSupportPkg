CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3 -DDEBUG

all: AppleEfiSignTool

AppleEfiSignTool: AppleEfiBinary.o Sha256.o Rsa2048Sha256.o main.o
	$(CC) Rsa2048Sha256.o Sha256.o AppleEfiBinary.o main.o -o AppleEfiSignTool

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o AppleEfiSignTool
