.PHONY: run
ARGS ?=

webserver: webserver.c
	$(CC) -o $@ $^

run: webserver
	./$< $(ARGS)
