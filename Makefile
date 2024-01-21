VERSION = 0.2.0

CFLAGS  = --std=c11 -pedantic
CFLAGS += -Os
CFLAGS += -ggdb
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -Iraylib/src
CFLAGS += -D_XOPEN_SOURCE=700
CFLAGS += -DVERSION='"$(VERSION)"'
LIBS = -Lraylib/src -lraylib -lm

OBJ = src/main.o

skin-view: $(OBJ)
	@printf 'CCLD\t%s\n' '$@'
	@$(CC) -o $@ $(OBJ) $(LIBS)

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	@printf 'CC\t%s\n' '$@'
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f skin-view $(OBJ)

.PHONY: clean
