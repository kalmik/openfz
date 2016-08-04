CC=gcc
CFLAGS=-lpthread -lreadline -Wall -pedantic

SRC=src
DIST=dist
BIN=$(DIST)/bin
LIB=$(DIST)/lib

DEPS=fuzzy-loader fuzzy-core
OBJ=$(DIST)/fuzzy-loader.o $(DIST)/fuzzy-core.o


all: openfz

openfz: $(DEPS)
	$(CC) $(SRC)/openfz.c $(OBJ) -o $(BIN)/openfz $(CFLAGS)

debug: $(DEPS)
	$(CC) $(SRC)/openfz.c $(OBJ) -o $(BIN)/openfz $(CFLAGS) -DDEBUG
	$(BIN)/openfz

fuzzy-loader:
	$(CC) -c $(SRC)/fuzzy-loader.c -o $(DIST)/fuzzy-loader.o

fuzzy-core:
	$(CC) -c $(SRC)/fuzzy-core.c -o $(DIST)/fuzzy-core.o

clean:
		rm $(DIST)/*.o

