all: client379 server379
	
server379:server.o server.h
	gcc -pthread -o server379 server.o
server.o:server.c server.h
	gcc -pthread -c server.c

client379: client.o client.h
	gcc -pthread -o client379 client.o

client.o:client.c client.h
	gcc -pthread -c client.c

clean:
	rm -f client.o
	rm -f client379
	rm -f server.o
	rm -f server379
