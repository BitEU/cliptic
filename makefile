# Makefile for Windows Cliptic

# Compiler settings
CC = cl
CFLAGS = /W3 /O2 /MT /D_CRT_SECURE_NO_WARNINGS /I.
LDFLAGS = /link
LIBS = user32.lib shell32.lib

# Output
TARGET = cliptic.exe

# Source files
SRCS = main.c \
       screen.c \
       config.c \
       database.c \
       terminal.c \
       interface.c \
       windows.c \
       puzzle.c \
       game.c \
       menus.c \
       utils.c

# Object files
OBJS = $(SRCS:.c=.obj)

# Build rules
all: $(TARGET)

$(TARGET): $(OBJS) sqlite3.obj
	$(CC) $(OBJS) sqlite3.obj $(LDFLAGS) $(LIBS) libcurl.lib sqlite3.lib /out:$@

.c.obj:
	$(CC) $(CFLAGS) /c $<

# SQLite3 (you'll need to download sqlite3.c and sqlite3.h)
sqlite3.obj: sqlite3.c
	$(CC) /c /O2 sqlite3.c

# Clean
clean:
	del *.obj
	del $(TARGET)

# Dependencies
main.obj: main.c cliptic.h terminal.h config.h database.h screen.h
screen.obj: screen.c screen.h cliptic.h interface.h
config.obj: config.c config.h interface.h
database.obj: database.c database.h sqlite3.h
terminal.obj: terminal.c terminal.h cliptic.h screen.h config.h database.h game.h
interface.obj: interface.c interface.h screen.h config.h database.h
windows.obj: windows.c windows.h screen.h config.h
puzzle.obj: puzzle.c puzzle.h config.h
game.obj: game.c game.h screen.h config.h menus.h
menus.obj: menus.c menus.h interface.h database.h screen.h game.h
utils.obj: utils.c cliptic.h