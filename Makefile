# TwinCAT Navigator Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -luser32 -lkernel32
SRCDIR = lib
OBJDIR = obj

# Hlavní cíl
navigator.exe: navigator.c $(SRCDIR)/twincat_navigator.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Debug verze
debug: CFLAGS += -DDEBUG_MODE -g
debug: navigator.exe

# Vyčistit
clean:
	del /Q *.exe *.o 2>nul || true

# Spustit
run: navigator.exe
	./navigator.exe

.PHONY: clean run debug