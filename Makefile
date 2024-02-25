CC = cc
CFLAGS = -Wall -Wimplicit-fallthrough -Wextra -Werror -std=c99 -pedantic -g

bin/turc: src/turc.c
	$(CC) $(CFLAGS) -o bin/turc src/turc.c
