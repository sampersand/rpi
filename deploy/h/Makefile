CFLAGS = -Wall -Wextra -Wpedantic -O3 -DNDEBUG #// -g -fsanitize=address,undefined
EXE = h

.PHONY: install clean test

$(EXE): *.c

install:
	cp $(EXE) /usr/local/bin/$(EXE)

clean:
	rm -f *.o $(EXE) /usr/local/bin/$(EXE)

test: $(EXE)
	./run-tests.rb $(EXE)
