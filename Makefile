all: run

server:
	gcc server.c lib.c -o server

subscriber:
	gcc client.c lib.c -o subscriber -lm

clean:
	rm -rf server
	rm -rf subscriber