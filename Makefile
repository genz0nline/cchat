all: cchat

SRC=src/cchat.c src/log.c src/terminal.c src/input.c src/err.c src/abuf.c src/tui.c src/network.c src/proto.c src/utils.c src/state.c

cchat: $(SRC)
	$(CC) -o cchat $(SRC) -s -lm

clean:
	rm -f src/*.o src/*.d cchat
