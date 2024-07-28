CC = x86_64-w64-mingw32-gcc
INCLUDE = -Iraylib/x86_64-w64-mingw32/include
LIBS = -Lraylib/x86_64-w64-mingw32/lib -l:libraylib.a -lgdi32 -lwinmm

TARGET = skin-view.exe
BUNDLE = src/bundle.exe

ARCHIVE = zip -r
ARCHIVE_EXT = .zip
