CC = x86_64-w64-mingw32-gcc
INCLUDE = -I.build -Iraylib/include
LIBS = -Lraylib/lib/x86_64-w64-mingw32 -l:libraylib.a -lgdi32 -lwinmm

TARGET = skin-view.exe
BUNDLE = .build/bundle.exe

ARCHIVE = zip -r
ARCHIVE_EXT = .zip
