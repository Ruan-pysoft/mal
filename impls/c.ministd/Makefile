# Adapted from impls/c.2/Makefile and impls/c/Makefile
# as well as examples/Makefile from Ruan-pysoft/ministd

CC            := gcc
CFLAGS        := -ffreestanding -fno-builtin -nostdlib -std=c89 -pedantic \
                 -I../../../mystdlib/include/ -Wall -Wextra \
                 -Wno-builtin-declaration-mismatch -g -O0
# for now do -O0, because -O2 makes debugging with gdb hell

LDFLAGS       := -L. -lministd

#####################

SRC = values.c reader.c printer.c error.c
HEADERS = values.h reader.h printer.h error.h

S0 = step0_repl
S1 = step1_read_print

S0_SRC = $(S0).c
S1_SRC = $(S1).c $(SRC)

S0_HEADERS =
S1_HEADERS = $(HEADERS)

#####################

.PHONY: all
all: $(S0) $(S1)

$(S0): $(S0_SRC) $(S0_HEADERS)
	$(CC) $(CFLAGS) $(S0_SRC) -o $@ $(LDFLAGS)

$(S1): $(S1_SRC) $(S1_HEADERS)
	$(CC) $(CFLAGS) $(S1_SRC) -o $@ $(LDFLAGS)

clean:
	rm -f $(S0) $(S1)
