CC=gcc
CFLAGS=-lpthread -lreadline -Wall -pedantic

SRC=src
DIST=dist
BIN=$(DIST)/bin
LIB=$(DIST)/lib

DEPS=fuzzy-loader fuzzy-core mod-fis logger openfz-cli
OBJ=$(DIST)/fuzzy-loader.o $(DIST)/fuzzy-core.o $(DIST)/mod-fis.o $(DIST)/logger.o


all: openfz

openfz: $(DEPS)
	$(CC) $(SRC)/openfz.c $(OBJ) -o $(BIN)/openfz $(CFLAGS)

openfz-cli:
	$(CC) $(SRC)/openfz-cli.c -o $(BIN)/openfz-cli $(CFLAGS)

debug: $(DEPS)
	killall openfz &
	$(CC) $(SRC)/openfz.c $(OBJ) -o $(BIN)/openfz $(CFLAGS) -DDEBUG
	$(BIN)/openfz

daemon: $(DEPS)
	$(CC) $(SRC)/openfz.c $(OBJ) -o $(BIN)/openfz $(CFLAGS) -DDEBUG
	$(BIN)/openfz -d &

fuzzy-loader:
	$(CC) -c $(SRC)/fuzzy-loader.c -o $(DIST)/fuzzy-loader.o

fuzzy-core:
	$(CC) -c $(SRC)/fuzzy-core.c -o $(DIST)/fuzzy-core.o

mod-fis:
	$(CC) -c $(SRC)/mod-fis.c -o $(DIST)/mod-fis.o

logger:
	$(CC) -c $(SRC)/logger.c -o $(DIST)/logger.o

clean:
		rm $(DIST)/*.o

