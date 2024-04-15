.POSIX:
.PRAGMA: target_name

include config.mk

VERSION = 0.3.0

CFLAGS = \
	--std=c11 -pedantic \
	-Os -Wl,s \
	-Wall -Wextra -Werror \
	-D_XOPEN_SOURCE=700 \
	-DVERSION='"$(VERSION)"'

OBJ = src/main.o

$(TARGET): $(OBJ)
	@printf 'CCLD\t%s\n' '$@'
	@$(CC) -o $@ $(OBJ) $(LIBS)

$(BUNDLE): src/bundle.c
	@printf 'CCLD\t%s\n' '$@'
	@$(CC) -o $@ $<

src/bundle.h: $(BUNDLE)
	@printf 'BUNDLE\t%s\n' '$@'
	@./$(BUNDLE)

src/main.o: src/bundle.h

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	@printf 'CC\t%s\n' '$@'
	@$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(TARGET) $(BUNDLE) src/bundle.h $(OBJ)

dist: $(TARGET)
	mkdir -p skin-view-$(VERSION)
	cp -t skin-view-$(VERSION) -r \
		$(TARGET) resources
	$(ARCHIVE) skin-view-$(VERSION)$(ARCHIVE_EXT) skin-view-$(VERSION)
	rm -rf skin-view-$(VERSION)
