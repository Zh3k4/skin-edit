#include "build.h"

#if defined(_WIN32) && defined(_MSC_VER)

const char *const cflags[] = {
	"/std:c11", "/Os", "/Wall"
};
const char *const libs[] = {
	"/LIBPATH:raylib/lib/x86_64-w64-msvc",
	"raylib.lib", "gdi32.lib", "winmm.lib", "user32.lib", "shell32.lib"
};

const char *const include[] = { "/Iraylib/include" };
const char *const ldflags[] = {
	"/link", "/SUBSYSTEM:WINDOWS", "/entry:mainCRTStartup"
};

#else

const char *const cflags[] = {
	"--std=c11", "-pedantic",
	"-Os",
	"-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror",
	"-D_XOPEN_SOURCE=700"
};
	#if defined(__MINGW32__) || defined(_USE_MINGW)
const char *const libs[] = {
	"-Lraylib/lib/x86_64-w64-mingw32",
	"-l:libraylib.a", "-lgdi32", "-lwinmm"
};
	#else
const char *const libs[] = {
	"-Lraylib/lib/x86_64-linux-gnu",
	"-l:libraylib.a", "-lm"
};
	#endif

const char *const include[] = { "-Iraylib/include" };
const char *const ldflags[] = { "-s" };

#endif

enum target {
	T_BUNDLE,
	T_BUNDLE_H,
	T_SKIN_VIEW,
};

bool
build_bundle(struct object o)
{
	struct command c;
	printf("CCLD\t%s\n", o.output);

	c = command_init(4);
	command_append(&c, CC, 0);
	command_append(&c, "-o", o.output, 0);
	command_append(&c, *o.inputs, 0);

	return proc_wait(proc_run(&c));
}

bool
build_bundle_h(struct object o)
{
	printf("BUNDLE\t%s\n", o.output);
	struct command c = command_init(1);
	command_append(&c, *o.inputs, 0);
	return proc_wait(proc_run(&c));
}

bool
build_target(struct object o)
{
	struct command c;
	printf("CCLD\t%s\n", o.output);

	c = command_init(count(cflags) + count(libs) + 6);
	command_append(&c, CC, *include, 0);
	for (usize i = 0; i < count(cflags); i++) {
		command_append(&c, cflags[i], 0);
	}

	command_append(&c, "-o", o.output, "src/main.c", 0);

	for (usize i = 0; i < count(ldflags); i++) {
		command_append(&c, ldflags[i], 0);
	}
	for (usize i = 0; i < count(libs); i++) {
		command_append(&c, libs[i], 0);
	}

	return proc_wait(proc_run(&c));
}

bool
build(struct object *obj, usize count)
{
	bool status = true;
	for (usize i = 0; i < count; i++) {
		struct object o = obj[i];
		if (!object_needs_rebuild(&o)) continue;

		bool result;
		switch (o.type) {
		case T_BUNDLE:
			result = build_bundle(o); break;
		case T_BUNDLE_H:
			result = build_bundle_h(o); break;
		case T_SKIN_VIEW:
			result = build_target(o); break;
		}
		status = status && result;
	}
	return status;
}

void
usage(bool err)
{
	fprintf(err ? stderr : stdout,
		"Usage: ./build [build|clean|help]\n");
}

void
clean(void)
{
	const char *const files[] = {
		"src/bundle",
		"src/bundle.exe",
		"src/bundle.h",
		"skin-view",
		"skin-view.exe",
	};

	for (usize i = 0; i < count(files); i++) {
		remove(files[i]);
	}
}

int
main(int argc, char **argv)
{

#if defined(_WIN32) || defined(_USE_MINGW)
	#define TARGET "skin-view.exe"
	#define BUNDLE "src/bundle.exe"
#else
	#define TARGET "skin-view"
	#define BUNDLE "src/bundle"
#endif

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

	struct object obj[] = {
		object_init(T_BUNDLE, BUNDLE,
			(const char*[]){ "src/bundle.c", 0}),
		object_init(T_BUNDLE_H, "src/bundle.h",
			(const char*[]){ BUNDLE, 0}),
		object_init(T_SKIN_VIEW, TARGET,
			(const char*[]){ "src/main.c", "src/bundle.h", 0}),
	};

	bool status = build(obj, count(obj));

	return status ? 0 : 1;
}
