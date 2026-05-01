CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined
EXE = h

.PHONY: install clean test

$(EXE): *.c

install:
	cp $(EXE) /usr/local/bin/$(EXE)

clean:
	rm -f *.o $(EXE) /usr/local/bin/$(EXE)

test:
	./run-tests.rb $(EXE)
