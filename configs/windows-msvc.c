bool
build_bundle(void)
{
	const char *o = ".build\\bundle.exe";
	const char *s = "src\\bundle.c";
	if (!target_needs_rebuild(o, &s, 1)) return true;

	printf("CCLD\t%s\n", o);
	char **c = NULL;
	arena_save();
	vec_add_many(&c, CC, "Fo.build\\", "/Fe:", o, s, 0);
	bool result = proc_wait(proc_run(c));
	arena_load();

	return result;
}

bool
build_bundle_h(void)
{
	const char *o = ".build\\bundle.h";
	const char *s = ".build\\bundle.exe";
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

	/* https://github.com/tsoding/musializer/blob/master/src_build/nob_win64_msvc.c */
	/* https://github.com/raysan5/raylib/wiki/Working-on-Windows */

	printf("CCLD\t%s\n", o);
	char **c = NULL;

	arena_save();
	vec_add_many(&c, CC, "/std:c11", "/Os", "/Wall", 0);
	vec_add_many(&c, "/D_WINSOCK_DEPRECATED_NO_WARNINGS", 0);
	vec_add_many(&c, "/D_CRT_SECURE_NO_WARNINGS", 0);
	vec_add_many(&c, "/Iraylib\\include", "/I.build", 0);
	vec_add_many(&c, "Fo.build\\", "/Fe:", o, s, 0);

	vec_add_many(&c, "/link", "/NODEFAULTLIB:libcmt", 0);
	vec_add_many(&c, "/SUBSYSTEM:WINDOWS", 0);
	vec_add_many(&c, "/LIBPATH:raylib\\lib\\x86_64-w64-msvc", 0);
	vec_add_many(&c, "raylib.lib", "Winmm.lib", 0);
	vec_add_many(&c, "gdi32.lib", "msvcrt.lib", 0);
	vec_add_many(&c, "user32.lib", "shell32.lib", 0);

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
	vec_add(&c, ".\\skin-view.exe");
	bool result = proc_wait(proc_run(c));
	arena_load();
	return result;
}
