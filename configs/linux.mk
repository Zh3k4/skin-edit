INCLUDE = -I.build -Iraylib/include
LIBS = -Lraylib/lib/x86_64-linux-gnu -l:libraylib.a -lm

TARGET = skin-view
BUNDLE = .build/bundle

ARCHIVE = tar czf
ARCHIVE_EXT = .tar.gz
