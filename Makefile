all:	client server

client:	client.c
	gcc client.c allStats.c -o client -Wall

server:	server.c
	gcc server.c allStats.c -o server -Wall


clean:
	rm -f client server
