all: cchat-client cchat-server

COMMON_DEPS = utils.c
SERVER_DEPS = server.c
CLIENT_DEPS = client.c
SERVER_DEPS += $(COMMON_DEPS)
CLIENT_DEPS += $(COMMON_DEPS)


cchat-server: $(SERVER_DEPS)
	$(CC) -o cchat-server $(SERVER_DEPS)

cchat-client: $(CLIENT_DEPS)
	$(CC) -o cchat-client $(CLIENT_DEPS)

clean:
	rm -f *.o *.d cchat-client cchat-server
