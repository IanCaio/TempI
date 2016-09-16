CFLAGS=-DNO_DEBUG

SOURCE=src/TempI.c
DEST=bin/TempI

all: src/TempI.c src/debugger.h src/TempI.h
	$(CC) $(CFLAGS) -o $(DEST) $(SOURCE) `pkg-config --libs --cflags appindicator-0.1`

debug: CFLAGS=-g
debug: src/TempI.c src/debugger.h src/TempI.h
	$(CC) $(CFLAGS) -o $(DEST) $(SOURCE) `pkg-config --libs --cflags appindicator-0.1`

install: all
	install -d ~/Executables/TempI/
	install -d ~/Executables/TempI/Icons/
	install -d ~/Executables/TempI/Config/
	install $(DEST) ~/Executables/TempI/
	install bin/Config/TempI.conf ~/Executables/TempI/Config/

