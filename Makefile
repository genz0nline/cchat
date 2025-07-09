all: cchat-client cchat-server

cchat-client: client.c
	$(CC) -o cchat-client client.c

cchat-server: server.c
	$(CC) -o cchat-server server.c

clean:
	rm -f *.o *.d cchat-client cchat-server
