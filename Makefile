

client: client.o
	gcc -o client client.o
client.o:client.c
	gcc -c client.c

clean:
	rm -f client.o
	rm -f client