all: cchat

SRC=src/cchat.c src/log.c

cchat: $(SRC)
	$(CC) -o cchat $(SRC)

clean:
	rm -f src/*.o src/*.d cchat
