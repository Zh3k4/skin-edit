CC = x86_64-w64-mingw32-gcc
INCLUDE = -Iraylib/include
LIBS = -Lraylib/lib/x86_64-w64-mingw32 -l:libraylib.a -lgdi32 -lwinmm

TARGET = skin-view.exe
BUNDLE = src/bundle.exe

ARCHIVE = zip -r
ARCHIVE_EXT = .zip
