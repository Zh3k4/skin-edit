const char *deps[] = {
	"src\\main.c",
};

bool
build_bundle(void)
{
	const char *o = ".build\\bundle.exe";
	const char *s = "src\\bundle.c";
	if (!target_needs_rebuild(o, &s, 1)) {
		return true;
	}
	printf("CCLD\t%s\n", o);
	struct command c = command_init(4);
	command_append(&c, CC, "/Fe:", o, s ,0);
	return proc_wait(proc_run(&c));
}

bool
build_bundle_h(void)
{
	const char *o = ".build\\bundle.h";
	const char *s = ".build\\bundle.exe";
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

	/* https://github.com/tsoding/musializer/blob/master/src_build/nob_win64_msvc.c */
	/* https://github.com/raysan5/raylib/wiki/Working-on-Windows */

	printf("CCLD\t%s\n", o);
	struct command c = command_init(20 + count(deps));
	command_append(&c, CC, "/std:c11", "/Os", "/Wall", 0);
	command_append(&c, "/D_WINSOCK_DEPRECATED_NO_WARNINGS", 0);
	command_append(&c, "/D_CRT_SECURE_NO_WARNINGS", 0);
	command_append(&c, "/Iraylib\\include", "/I.build", 0);
	command_append(&c, "/Fe:", o, 0);

	command_append_vec(&c, deps, count(deps));

	command_append(&c, "/link", "/NODEFAULTLIB:libcmt", 0);
	command_append(&c, "/SUBSYSTEM:WINDOWS", 0);
	command_append(&c, "/LIBPATH:raylib\\lib\\x86_64-w64-msvc", 0);
	command_append(&c, "raylib.lib", "Winmm.lib", 0);
	command_append(&c, "gdi32.lib", "msvcrt.lib", 0);
	command_append(&c, "user32.lib", "shell32.lib", 0);

	return proc_wait(proc_run(&c));
}

bool
build(void)
{
	return build_bundle()
		&& build_bundle_h()
		&& build_target();
}

