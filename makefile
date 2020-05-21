all:
		gcc -o srv server.c
		gcc -o wrk -pthread woreker.c -lm