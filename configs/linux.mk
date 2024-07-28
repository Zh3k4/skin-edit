INCLUDE = -Iraylib/x86_64-linux-gnu/include
LIBS = -Lraylib/x86_64-linux-gnu/lib -l:libraylib.a -lm

TARGET = skin-view
BUNDLE = src/bundle

ARCHIVE = tar czf
ARCHIVE_EXT = .tar.gz
