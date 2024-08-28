bool
build_bundle(void)
{
	const char *o = ".build/bundle";
	const char *s = "src/bundle.c";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	xfprintf(stdout, "CCLD\t%s\n", o);
	char **c = NULL;
	size_t save = arena.count;
	vec_add_many(&c, CC, "-o", o, s, 0);
	bool result = proc_wait(proc_run(c));
	arena.count = save;

	return result;
}

bool
build_bundle_h(void)
{
	const char *o = ".build/bundle.h";
	const char *s = ".build/bundle";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	xfprintf(stdout, "BUNDLE\t%s\n", o);
	char **c = NULL;
	size_t save = arena.count;
	vec_add(&c, (char*)s);
	bool result = proc_wait(proc_run(c));
	arena.count = save;

	return result;
}

bool
build_target(void)
{
	const char *o = "skin-view";
	const char *s = "src/main.c";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	xfprintf(stdout, "CCLD\t%s\n", o);
	char **c = NULL;

	size_t save = arena.count;
	vec_add_many(&c, CC, "--std=c11", "-pedantic", "-Os", 0);
	vec_add_many(&c, "-Wall", "-Wextra", "-Wshadow", 0);
	vec_add_many(&c, "-Wconversion", "-Werror", 0);
	vec_add_many(&c, "-D_XOPEN_SOURCE=700", 0);
	vec_add_many(&c, "-Iraylib/include", "-I.build", 0);
	vec_add_many(&c, "-o", o, s, 0);
	vec_add_many(&c, "-s", "-Lraylib/lib/x86_64-linux-gnu", 0);
	vec_add_many(&c, "-l:libraylib.a", "-lm", 0);

	bool result = proc_wait(proc_run(c));
	arena.count = save;
	return result;
}

bool
build(void)
{
	return build_bundle()
		&& build_bundle_h()
		&& build_target();
}

bool
run(void)
{
	char **c = NULL;
	size_t save = arena.count;
	vec_add(&c, "./skin-view");
	bool result = proc_wait(proc_run(c));
	arena.count = save;
	return result;
}
