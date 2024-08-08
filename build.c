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
usage(bool err)
{
	fprintf(err ? stderr : stdout,
		"Usage: ./build [build|clean|help]\n");
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

	if (argc == 2 && !strcmp(argv[1], "help")) {
		usage(false);
		return 0;
	}
	else if (argc == 2 && !strcmp(argv[1], "clean")) {
		clean();
		return 0;
	}
	else if (!(argc == 1 || (argc == 2 && !strcmp(argv[1], "build")))) {
		usage(true);
		return 1;
	}

	create_dir(".build");
	return build() ? 0 : 1;
}
