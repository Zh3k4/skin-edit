#include "build.h"

#if defined(_WIN32)
#	if defined(__GNUC__)
#		define CC "gcc"
#	elif defined(__clang__)
#		define CC "clang"
#	elif defined(_MSC_VER)
#		define CC "cl"
#	elif defined(__MINGW32__)
#		define CC "x86_64-w64-mingw32-gcc"
#	endif
#elif defined(__MINGW32__)
#	define CC "x86_64-w64-mingw32-gcc"
#else
#	define CC "cc"
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#	include "configs/windows-msvc.c"
#elif defined(_WIN32) || defined(__MINGW32__)
#	include "configs/windows-mingw.c"
#else
#	include "configs/linux.c"
#endif

void
usage(void)
{
	xfprintf(stderr, "Usage: ./build [build|run|clean|help]\n");
}

void
clean(void)
{
	remove_dir(".build");
	remove_file("skin-view");
	remove_file("skin-view.exe");
}

int
main(int argc, char **argv)
{
	if (argc == 1) {
		create_dir(".build");
		return build() ? 0 : 1;
	}

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "clean")) {
			clean();
		}
		else if (!strcmp(argv[i], "build")) {
			create_dir(".build");
			if (!build()) return 1;
		}
		else if (!strcmp(argv[i], "run")) {
			create_dir(".build");
			if (!build()) return 1;
			if (!run()) return 1;
		}
		else if (!strcmp(argv[i], "help")) {
			usage();
			return 0;
		}
		else {
			usage();
			return 1;
		}
	}
	return 0;
}
