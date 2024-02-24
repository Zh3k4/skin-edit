CC = x86_64-w64-mingw32-gcc
INCLUDE = -Iraylib-5.0_win64_mingw-w64/include
LIBS = -Lraylib-5.0_win64_mingw-w64/lib -lraylib -lgdi32 -lwinmm

TARGET = skin-view.exe

ARCHIVE = zip -r
ARCHIVE_EXT = .zip
