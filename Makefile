all: client server
server:server.o
	gcc -o server server.o
server.o:server.c
	gcc -c server.c

client: client.o
	gcc -pthread -o client client.o
client.o:client.c
	gcc -pthread -c client.c

clean:
	rm -f client.o
	rm -f client
	rm -f server.o
	rm -f server
