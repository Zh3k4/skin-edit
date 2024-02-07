CC = x86_64-w64-mingw32-gcc
INCLUDE = -Iraylib-mingw/include
LIBS = -Lraylib-mingw/lib -lraylib -lgdi32 -lwinmm

TARGET = skin-view.exe

ARCHIVE = zip -r
ARCHIVE_EXT = .zip
