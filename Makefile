include config.mk

VERSION = 0.6.0

CFLAGS = \
	--std=c11 -pedantic \
	-Os \
	-Wall -Wextra -Wshadow -Wconversion -Werror \
	-DNDEBUG -D_XOPEN_SOURCE=700
LDFLAGS = -s

OBJ = .build/main.o

$(TARGET): $(OBJ)
	@printf 'CCLD\t%s\n' '$@'
	@$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(BUNDLE): src/bundle.c
	@mkdir -p .build
	@printf 'CCLD\t%s\n' '$@'
	@$(CC) -o $@ $<

.build/bundle.h: $(BUNDLE)
	@printf 'BUNDLE\t%s\n' '$@'
	@./$(BUNDLE)

.build/main.o: .build/bundle.h

.SUFFIXES:
.SUFFIXES: .c .o

.build/%.o: src/%.c
	@mkdir -p .build
	@printf 'CC\t%s\n' '$@'
	@$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -r -- $(TARGET) .build

dist: $(TARGET)
	mkdir -p skin-view-$(VERSION)
	cp -t skin-view-$(VERSION) $(TARGET)
	$(ARCHIVE) skin-view-$(VERSION)$(ARCHIVE_EXT) skin-view-$(VERSION)
	rm -rf skin-view-$(VERSION)
