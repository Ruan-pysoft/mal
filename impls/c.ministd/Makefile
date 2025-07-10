# Adapted from impls/c.2/Makefile and impls/c/Makefile
# as well as examples/Makefile from Ruan-pysoft/ministd

CC            := gcc
CFLAGS        := -ffreestanding -fno-builtin -nostdlib -std=c89 -pedantic \
                 -I../../../mystdlib/include/ -Wall -Wextra \
                 -Wno-builtin-declaration-mismatch -g -O2

LDFLAGS       := -L. -lministd

#####################

SRC = 
HEADERS = 

S0 = step0_repl

S0_SRC = $(S0).c

S0_HEADERS =

#####################

.PHONY: all
all: $(S0)

$(S0): $(S0_SRC) $(S0_HEADERS)
	$(CC) $(CFLAGS) $(S0_SRC) -o $(S0) $(LDFLAGS)

clean:
	rm -f $(S0)
