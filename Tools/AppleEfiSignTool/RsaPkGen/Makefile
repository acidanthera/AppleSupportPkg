CC ?= gcc
CFLAGS=-c -Wall -Wextra -pedantic -O3 -I/usr/local/opt/openssl/include -lcrypto
LDFLAGS=-L/usr/local/opt/openssl/lib -lcrypto

all: RsaTool

RsaTool: RsaTool.o
	$(CC) RsaTool.o -o RsaTool $(LDFLAGS)

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o RsaTool
