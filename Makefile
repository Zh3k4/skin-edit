CFLAGS  = --std=c11 -pedantic
CFLAGS += -Os
CFLAGS += -Wall -Wextra -Werror
CFLAGS += -Iraylib/src
CFLAGS += -D_XOPEN_SOURCE=700
LIBS = -Lraylib/src -lraylib -lm

OBJ = src/main.o

skin-edit: $(OBJ)
	@printf 'CCLD\t%s\n' '$@'
	@$(CC) -o $@ $(OBJ) $(LIBS)

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	@printf 'CC\t%s\n' '$@'
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f skin-edit $(OBJ)

.PHONY: clean
