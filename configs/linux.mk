INCLUDE = -Iraylib/include
LIBS = -Lraylib/lib/x86_64-linux-gnu -l:libraylib.a -lm

TARGET = skin-view
BUNDLE = src/bundle

ARCHIVE = tar czf
ARCHIVE_EXT = .tar.gz
