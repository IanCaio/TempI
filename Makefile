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
	install -m 664 bin/Config/TempI.config ~/Executables/TempI/Config/
	install -m 664 bin/Icons/menu-icon.png ~/Executables/TempI/Icons/
	install -m 664 bin/Icons/green-icon.png ~/Executables/TempI/Icons/
	install -m 664 bin/Icons/orange-icon.png ~/Executables/TempI/Icons/
	install -m 664 bin/Icons/yellow-icon.png ~/Executables/TempI/Icons/
	install -m 664 bin/Icons/red-icon.png ~/Executables/TempI/Icons/

