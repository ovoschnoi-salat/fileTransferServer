CFLAGS=--std=c99 -Wall -pedantic -Isrc/ -ggdb -Wextra -DDEBUG
BUILDDIR=build
CLIENTDIR=clientSrc
SERVERDIR=serverSrc
CC=gcc

all: server client

server: $(BUILDDIR)/$(SERVERDIR)/main.o
	$(CC) -o server $^

client: $(BUILDDIR)/$(CLIENTDIR)/main.o
	$(CC) -o client $^

$(BUILDDIR)/$(CLIENTDIR)/main.o: $(CLIENTDIR)/main.c
	mkdir -p $(BUILDDIR)/$(CLIENTDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILDDIR)/$(SERVERDIR)/main.o: $(SERVERDIR)/main.c
	mkdir -p $(BUILDDIR)/$(SERVERDIR)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR) client server