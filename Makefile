CC = gcc
CFLAGS = -Wall -Wextra -g

all: central

debug: CFLAGS += -DDEBUG -g
debug: all

central: central.c central.h common.h
		$(CC) $(CFLAGS) central.c -o central

estafeta:
		@echo "estafeta TBD"

gerador:
		@echo "gerador TBD"

clean:
		rm -f central estafeta gerador *.o ./central.lock