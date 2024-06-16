#include "build.h"

#define CFLAGS \
	"--std=c11", "-pedantic", \
	"-Os", \
	"-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror", \
	"-D_XOPEN_SOURCE=700", \
	"-DVERSION=\"0.4.0\""
#define INCLUDE "-Iraylib/src"
#define LDFLAGS "-s"

#if defined(_WIN32) || defined(_USE_MINGW)
#	define TARGET "skin-view.exe"
#	define BUNDLE "src/bundle.exe"
#	define LIBS "-Lraylib/src/", "-lraylib", "-lgdi32", "-lwinmm"
#else
#	define TARGET "skin-view"
#	define BUNDLE "src/bundle"
#	define LIBS "-Lraylib/src/", "-lraylib", "-lm"
#endif

const char *const obj[] = {
	"src/main.o",
	NULL,
};

i8
make(char *prefix, char *output, char **deps, char **cmd)
{
	i8Result r = needs_rebuild(output, (const char**)deps);
	if (!r.ok) return 0;
	if (!r.val) return 1;

	printf("%s\t%s\n", prefix, output);
	Proc p = create_process(cmd);
	if (p == INVALID_PROC) return 0;
	if (!wait_for_process(p)) return 0;
	return 1;
}

i8
c2o(char *ofile)
{
	assert(ofile);

	char *cfile = strdup(ofile);
	assert(cfile);

	cfile[strlen(cfile) - 1] = 'c';

	i8 status = make("CC", ofile, (char *[]){ cfile, NULL }, (char *[]){
			CC, CFLAGS, INCLUDE, "-c", cfile, "-o", ofile, NULL
		});

	free(cfile);
	return status;
}

i8
skin_view(void)
{
	return make("CCLD", TARGET, (char *[]){ "src/main.o", NULL },
		(char *[]){
			CC, LDFLAGS, "src/main.o", "-o", TARGET, LIBS, NULL
		});
}

i8
build(void)
{
	i8 status = 1;
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

i8
remove_(const char *pathname)
{
	assert(pathname);
	errno = 0;
	if (remove(pathname) && errno != ENOENT) return 0;
	if (errno != ENOENT) printf("RM\t%s\n", pathname);
	return 1;
}

i8
clean(void)
{
	i8 status = remove_(TARGET)
		&& remove_(BUNDLE)
		&& remove_("src/bundle.h");

	for (usize i = 0; obj[i]; i += 1)
		status = status && remove_(obj[i]);

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
