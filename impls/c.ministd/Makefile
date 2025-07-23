# Adapted from impls/c.2/Makefile and impls/c/Makefile
# as well as examples/Makefile from Ruan-pysoft/ministd

CC            := gcc
CFLAGS        := -ffreestanding -fno-builtin -nostdlib -std=c89 -pedantic \
                 -I../../../mystdlib/include/ -Wall -Wextra \
                 -Wno-builtin-declaration-mismatch -g -O0
# for now do -O0, because -O2 makes debugging with gdb hell

LDFLAGS       := -L. -lministd

#####################

SRC = types.c values.c reader.c printer.c error.c env.c core.c
HEADERS = types.h values.h reader.h printer.h error.h env.h core.h

S0 = step0_repl
S1 = step1_read_print
S2 = step2_eval
S3 = step3_env
S4 = step4_if_fn_do

S0_SRC = $(S0).c
S1_SRC = $(S1).c types.c values.c reader.c printer.c error.c env.c
S2_SRC = $(S2).c types.c values.c reader.c printer.c error.c env.c
S3_SRC = $(S3).c types.c values.c reader.c printer.c error.c env.c
S4_SRC = $(S4).c $(SRC)

S0_HEADERS =
S1_HEADERS = types.h values.h reader.h printer.h error.h env.h
S2_HEADERS = types.h values.h reader.h printer.h error.h env.h
S3_HEADERS = types.h values.h reader.h printer.h error.h env.h
S4_HEADERS = $(HEADERS)

#####################

.PHONY: all
all: $(S0) $(S1) $(S2) $(S3) $(S4)

$(S0): $(S0_SRC) $(S0_HEADERS)
	$(CC) $(CFLAGS) $(S0_SRC) -o $@ $(LDFLAGS)

$(S1): $(S1_SRC) $(S1_HEADERS)
	$(CC) $(CFLAGS) $(S1_SRC) -o $@ $(LDFLAGS)

$(S2): $(S2_SRC) $(S2_HEADERS)
	$(CC) $(CFLAGS) $(S2_SRC) -o $@ $(LDFLAGS)

$(S3): $(S3_SRC) $(S3_HEADERS)
	$(CC) $(CFLAGS) $(S3_SRC) -o $@ $(LDFLAGS)

$(S4): $(S4_SRC) $(S4_HEADERS)
	$(CC) $(CFLAGS) $(S4_SRC) -o $@ $(LDFLAGS)

clean:
	rm -f $(S0) $(S1) $(S2) $(S3) $(S4)
