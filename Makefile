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

.SUFFIXES:
.SUFFIXES: .c .o

.c.o:
	@printf 'CC\t%s\n' '$@'
	@$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)

dist: $(TARGET)
	mkdir -p skin-view-$(VERSION)
	cp -t skin-view-$(VERSION) -r \
		$(TARGET) resources
	$(ARCHIVE) skin-view-$(VERSION)$(ARCHIVE_EXT) skin-view-$(VERSION)
	rm -rf skin-view-$(VERSION)
