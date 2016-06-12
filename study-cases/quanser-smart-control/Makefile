.SUFFIXES:
.SUFFIXES: .c .cpp

CC = gcc
GCC = g++

.c:
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(GL_LIBS) -o $@

.cpp:
	$(GCC) $< fis.cpp fuzzy-core.c fuzzy-loader.c -o $@ $(CFLAGS) -lpthread -lmosquitto -w


