#include "build.h"

#if defined(_WIN32) && defined(_MSC_VER)

const char *const cflags[] = {
	"/std:c11", "/pedantic",
	"/O:s",
	"/Wall", "/Werror",
	"/DVERSION=\"0.4.1\"", 0
};
const char *const libs[] = {
	"/LIBPATH:raylib/lib/x86_64-w64-msvc16",
	"raylib.lib", "gdi32.lib", "winmm.lib", 0
};

const char *const include[] = { "/Iraylib/include", 0 };
const char *const ldflags[] = { "/s", 0 };

#else

const char *const cflags[] = {
	"--std=c11", "-pedantic",
	"-Os",
	"-Wall", "-Wextra", "-Wshadow", "-Wconversion", "-Werror",
	"-D_XOPEN_SOURCE=700",
	"-DVERSION=\"0.4.1\"", 0
};
	#if defined(__MINGW32__) || defined(_USE_MINGW)
const char *const libs[] = {
	"-Lraylib/lib/x86_64-w64-mingw32",
	"-l:libraylib.a", "-lgdi32", "-lwinmm", 0
};
	#else
const char *const libs[] = {
	"-Lraylib/lib/x86_64-linux-gnu",
	"-l:libraylib.a", "-lm", 0
};
	#endif

const char *const include[] = { "-Iraylib/include", 0 };
const char *const ldflags[] = { "-s", 0 };

#endif

enum target {
	T_BUNDLE,
	T_BUNDLE_H,
	T_OBJECT,
	T_SKIN_VIEW,
};

bool
build_bundle(struct object o)
{
	struct command c;
	printf("CCLD\t%s\n", o.output);

#ifdef _MSC_VER
	c = command_init(3);
	command_append(&c, CC, 0);
	command_append(&c, temp_fmt("/out:%s", o.output), 0);
	command_append(&c, *o.inputs, 0);
#else
	c = command_init(4);
	command_append(&c, CC, 0);
	command_append(&c, "-o", o.output, 0);
	command_append(&c, *o.inputs, 0);
#endif

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
build_object(struct object o)
{
	struct command c;
	printf("CC\t%s\n", o.output);

#ifdef _MSC_VER
	c = command_init(count(cflags) + o.input_count + 3);
	command_append(&c, CC, 0);
	for (usize i = 0; i < count(cflags); i++) {
		command_append(&c, cflags[i], 0);
	}
	command_append(&c, *include, 0);
	command_append(&c, "/c", temp_fmt("/out:%s", o.output), 0);
	for (usize i = 0; i < o.input_count; i++) {
		command_append(&c, o.inputs[i], 0);
	}
#else
	c = command_init(count(cflags) + o.input_count + 4);
	command_append(&c, CC, 0);
	for (usize i = 0; i < count(cflags); i++) {
		command_append(&c, cflags[i], 0);
	}
	command_append(&c, *include, 0);
	command_append(&c, "-c", "-o", o.output, 0);

	char *input = string_replace_last_n_chars(o.output, ".c", 2);
	command_append(&c, input, 0);
#endif

	return proc_wait(proc_run(&c));
}

bool
build_target(struct object o)
{
	struct command c;
	printf("CCLD\t%s\n", o.output);

#ifdef _MSC_VER
	c = command_init(o.input_count + count(libs) + 3);
	command_append(&c, CC, *ldflags, 0);
	command_append(&c, temp_fmt("/out:%s", o.output), 0);
	for (usize i = 0; i < o.input_count; i++) {
		command_append(&c, o.inputs[i], 0);
	}
	for (usize i = 0; i < count(libs); i++) {
		command_append(&c, libs[i], 0);
	}
#else
	c = command_init(o.input_count + count(libs) + 4);
	command_append(&c, CC, *ldflags, 0);
	command_append(&c, "-o", o.output, 0);
	for (usize i = 0; i < o.input_count; i++) {
		command_append(&c, o.inputs[i], 0);
	}
	for (usize i = 0; i < count(libs); i++) {
		command_append(&c, libs[i], 0);
	}
#endif

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
		case T_OBJECT:
			result = build_object(o); break;
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
		"src/main.o",
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
		object_init(T_OBJECT, "src/main.o",
			(const char*[]){ "src/main.c", "src/bundle.h", 0}),
		object_init(T_SKIN_VIEW, TARGET,
			(const char*[]){ "src/main.o", 0}),
	};

	bool status = build(obj, count(obj));

	return status ? 0 : 1;
}
