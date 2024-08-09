bool
build_bundle(void)
{
	const char *o = ".build/bundle.exe";
	const char *s = "src/bundle.c";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	printf("CCLD\t%s\n", o);
	char **c = NULL;
	arena_save();
	vec_add_many(&c, CC, "-o", o, s, 0);
	bool result = proc_wait(proc_run(c));
	arena_load();

	return result;
}

bool
build_bundle_h(void)
{
	const char *o = ".build/bundle.h";
	const char *s = ".build/bundle.exe";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	printf("BUNDLE\t%s\n", o);
	char **c = NULL;
	arena_save();
	vec_add(&c, (char*)s);
	bool result = proc_wait(proc_run(c));
	arena_load();

	return result;
}

bool
build_target(void)
{
	const char *o = "skin-view.exe";
	const char *s = "src/main.c";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	printf("CCLD\t%s\n", o);
	char **c = NULL;

	arena_save();
	vec_add_many(&c, CC, "--std=c11", "-pedantic", "-Os", 0);
	vec_add_many(&c, "-Wall", "-Wextra", "-Wshadow", 0);
	vec_add_many(&c, "-Wconversion", "-Werror", 0);
	vec_add_many(&c, "-D_XOPEN_SOURCE=700", 0);
	vec_add_many(&c, "-Iraylib/include", "-I.build", 0);
	vec_add_many(&c, "-o", o, s, 0);
	vec_add_many(&c, "-s", "-Lraylib/lib/x86_64-w64-mingw32", 0);
	vec_add_many(&c, "-l:libraylib.a", "-lgdi32", "-lwinmm", 0);

	bool result = proc_wait(proc_run(c));
	arena_load();
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
	arena_save();
	vec_add(&c, "./skin-view.exe");
	bool result = proc_wait(proc_run(c));
	arena_load();
	return result;
}
