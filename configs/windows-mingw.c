const char *deps[] = {
	"src/main.c",
};

bool
build_bundle(void)
{
	const char *o = ".build/bundle.exe";
	const char *s = "src/bundle.c";
	if (!target_needs_rebuild(o, &s, 1)) {
		return true;
	}
	printf("CCLD\t%s\n", o);
	struct command c = command_init(4);
	command_append(&c, CC, "-o", o, s ,0);
	return proc_wait(proc_run(&c));
}

bool
build_bundle_h(void)
{
	const char *o = ".build/bundle.h";
	const char *s = ".build/bundle.exe";
	if (!target_needs_rebuild(o, &s, 1)) {
		return true;
	}

	printf("BUNDLE\t%s\n", o);
	struct command c = command_init(1);
	command_append(&c, s, 0);
	return proc_wait(proc_run(&c));
}

bool
build_target(void)
{
	const char *o = "skin-view.exe";
	if (!target_needs_rebuild(o, deps, count(deps))) {
		return true;
	}

	printf("CCLD\t%s\n", o);
	struct command c = command_init(19 + count(deps));
	command_append(&c, CC, "--std=c11", "-pedantic", "-Os", 0);
	command_append(&c, "-Wall", "-Wextra", "-Wshadow", 0);
	command_append(&c, "-Wconversion", "-Werror", 0);
	command_append(&c, "-D_XOPEN_SOURCE=700", 0);
	command_append(&c, "-Iraylib/include", "-I.build", 0);
	command_append(&c, "-o", o, 0);

	command_append_vec(&c, deps, count(deps));

	command_append(&c, "-s", "-Lraylib/lib/x86_64-w64-mingw32", 0);
	command_append(&c, "-l:libraylib.a", "-lgdi32", "-lwinmm", 0);

	return proc_wait(proc_run(&c));
}

bool
build(void)
{
	return build_bundle()
		&& build_bundle_h()
		&& build_target();
}

