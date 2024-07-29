#include "build.h"

#if defined(_WIN32) || defined(_USE_MINGW)
	#define TARGET "skin-view.exe"
	#define BUNDLE "src/bundle.exe"
	#if defined(_USE_MINGW)
		#define LIBS "-Lraylib/lib/x86_64-w64-mingw32", \
			"-l:libraylib.a", "-lgdi32", "-lwinmm"
	#else
		#define LIBS "/link", "/LIBPATH:raylib\lib\x86_64-w64-msvc16", \
			"raylib.lib", "gdi32.lib", "winmm.lib"
	#endif
#else
	#define TARGET "skin-view"
	#define BUNDLE "src/bundle"
	#define LIBS "-Lraylib/lib/x86_64-linux-gnu", \
		"-l:libraylib.a", "-lm"
#endif

#define CFLAGS \
	"--std=c11", "-pedantic", \
	"-Os", \
	"-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror", \
	"-D_XOPEN_SOURCE=700", \
	"-DVERSION=\"0.4.1\""
#define INCLUDE "-Iraylib/include"
#define LDFLAGS "-s"

const char *const obj[] = {
	"src/main.o",
	NULL,
};

bool
make(char *prefix, char *output, char **deps, char **cmd)
{
	int r = needs_rebuild(output, (const char**)deps);
	if (r < 0) return false;
	if (r == 0) return true;

	printf("%s\t%s\n", prefix, output);
	Proc p = create_process(cmd);
	if (p == INVALID_PROC) return false;
	if (!wait_for_process(p)) return false;
	return true;
}

bool
c2o(char *ofile)
{
	assert(ofile);

	char *cfile = strdup(ofile);
	assert(cfile);

	cfile[strlen(cfile) - 1] = 'c';

#ifdef _WIN32
	char *const outparam = calloc(5 + strlen(ofile) + 1, 1);
	sprintf(outparam, "/out:%s", ofile);

	bool status = make("CC", ofile, (char *[]){ cfile, NULL }, (char *[]){
			CC, CFLAGS, INCLUDE, "-c", cfile, outparam, NULL
		});
	free(outparam);
#else
	bool status = make("CC", ofile, (char *[]){ cfile, NULL }, (char *[]){
			CC, CFLAGS, INCLUDE, "-c", cfile, "-o", ofile, NULL
		});
#endif

	free(cfile);
	return status;
}

bool
skin_view(void)
{
	return make("CCLD", TARGET, (char *[]){ "src/main.o", NULL },
		(char *[]){
#ifdef _WIN32
			CC, LDFLAGS, "src/main.o", "/out:" TARGET, LIBS, NULL
#else
			CC, LDFLAGS, "src/main.o", "-o", TARGET, LIBS, NULL
#endif
		});
}

bool
build(void)
{
	bool status = 1;
	{
		char *deps[] = { "src/bundle.c", NULL };
		char *cmd[] = {
			CC, "-o", BUNDLE, "src/bundle.c", NULL
		};
		status = status && make("CCLD", BUNDLE, deps, cmd);
	}
	{
		char *deps[] = { BUNDLE, NULL };
		char *cmd[] = { BUNDLE, NULL };
		status = status && make("BUNDLE", "src/bundle.h", deps, cmd);
	}
	{
		char *deps[] = { "src/main.c", "src/bundle.h", NULL };
		char *cmd[] = { CC, CFLAGS, INCLUDE, "-c", "src/main.c", "-o", "src/main.o", NULL };
		status = status && make("CC", "src/main.o", deps, cmd);
	}
	return status && skin_view();
}

bool
remove_(const char *pathname)
{
	assert(pathname);
	errno = 0;
	if (remove(pathname) && errno != ENOENT) return false;
	return true;
}

bool
clean(void)
{
	bool status = true;

	status = status && remove_(TARGET);
	status = status && remove_(BUNDLE);
	status = status && remove_("src/bundle.h");

	for (usize i = 0; obj[i]; i += 1) {
		status = status && remove_(obj[i]);
	}

	return status;
}

enum Job {
	JOB_NONE,
	JOB_BUILD,
	JOB_CLEAN,
	JOB_DIST,
};

void
usage(int err)
{
	fprintf(err ? stderr : stdout,
		"Usage: build <build|clean|dist>\n");
}

int
main(int argc, char **argv)
{
	enum Job j = JOB_NONE;
	if (argc == 2 && !strcmp(argv[1], "help")) {
		usage(0);
		return 0;
	}
	else if (argc == 1 || (argc == 2 && !strcmp(argv[1], "build"))) {
		j = JOB_BUILD;
	}
	else if (argc == 2 && !strcmp(argv[1], "clean")) {
		j = JOB_CLEAN;
	}
	else if (argc == 2 && !strcmp(argv[1], "dist")) {
		j = JOB_DIST;
	}

	switch (j) {
	case JOB_BUILD:
		return !build();
	case JOB_CLEAN:
		return !clean();
	case JOB_DIST: assert(0 && "not implemented");
	default:
		usage(1);
		return 1;
	}
	return 0;
}
