#ifndef BUILD_H
#define BUILD_H

#if defined(WIN32) && !defined(_WIN32)
#	define _WIN32
#endif

#ifndef _WIN32
#	define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	define _WINUSER_
#	define _WINGDI_
#	define _IMM_
#	define _WINCON_
#	include <windows.h>
#	include <direct.h>
#	include <shellapi.h>
#else
#	include <dirent.h>
#	include <sys/stat.h>
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <unistd.h>
#endif

#ifdef _WIN32
	typedef HANDLE Proc;
#	define INVALID_PROC INVALID_HANDLE_VALUE
#	define SEP '\\'
#else
	typedef int Proc;
#	define INVALID_PROC (-1)
#	define SEP '/'
#endif

#define count(a) (sizeof(a)/sizeof(*(a)))
#define len(s)   (count(s) - 1)

#ifndef TEMP_MAX
#define TEMP_MAX 8 * 1024
#endif
char temporary[TEMP_MAX] = {0};
size_t temporary_count = 0;

struct str {
	size_t len;
	char *buf;
};

struct command {
	size_t size;
	size_t len;
	struct str *items;
};

void *
temp_alloc(size_t size)
{
	assert(size < TEMP_MAX);

	if (temporary_count + size > TEMP_MAX) {
		temporary_count = 0;
	}

	size_t count = temporary_count;
	memset(&temporary[count], 0, size);
	temporary_count += size;

	return &temporary[count];
}

char *
temp_fmt(const char *fmt, ...) {
	char *str;
	size_t len;
	va_list args, args_len;

	va_start(args, fmt);
	va_copy(args_len, args);
	len = vsnprintf(NULL, 0, fmt, args_len);
	va_end(args_len);

	/* TODO: better error handling, potentially */
	assert(len > 0);

	str = temp_alloc(len + 1);
	vsprintf(str, fmt, args);
	va_end(args);

	return str;
}

const char **
array_substitute(const char **array, const size_t len,
		const char *pattern, const char *replacement)
{
	assert(array && pattern && replacement);

	char *pa = strchr(pattern, '*');
	char *re = strchr(replacement, '*');
	assert(pa && re);

	size_t pa_prefix_len = pa - pattern;
	char *pa_prefix = temp_alloc(pa_prefix_len + 1);
	memcpy(pa_prefix, pattern, pa_prefix_len);

	size_t pa_suffix_len = strlen(pa + 1);
	char *pa_suffix = temp_alloc(pa_suffix_len + 1);
	memcpy(pa_suffix, &pattern[pa_prefix_len + 1], pa_suffix_len);

	size_t re_prefix_len = re - replacement;
	char *re_prefix = temp_alloc(re_prefix_len + 1);
	memcpy(re_prefix, replacement, re_prefix_len);

	size_t re_suffix_len = strlen(re + 1);
	char *re_suffix = temp_alloc(re_suffix_len + 1);
	memcpy(re_suffix, &replacement[re_prefix_len + 1], re_suffix_len);

	size_t z = 0;
	for (size_t i = 0; i < len; i++) {
		size_t l = strlen(array[i]);
		char *pre = strstr(array[i], pa_prefix);
		if (pre != array[i]) {
			z += l + 1;
			continue;
		}
		char *suf = strstr(array[i], pa_suffix);
		if (array[i] + l - suf != pa_suffix_len) {
			z += l + 1;
			continue;
		}

		size_t core_len = suf - pre - pa_prefix_len;
		z += re_prefix_len + core_len + re_suffix_len + 1;
	}

	char **newarr = temp_alloc(len * sizeof(*newarr) + z);
	char *data = (char *)&newarr[len];
	for (size_t i = 0; i < len; i++) {
		newarr[i] = data;

		size_t l = strlen(array[i]);
		char *pre = strstr(array[i], pa_prefix);
		if (pre != array[i]) {
			memcpy(data, array[i], l);
			data = &data[l + 1];
			continue;
		}
		char *suf = strstr(array[i], pa_suffix);
		if (array[i] + l - suf != pa_suffix_len) {
			memcpy(data, array[i], l);
			data = &data[l + 1];
			continue;
		}

		int core_len = suf - pre - pa_prefix_len;
		sprintf(data, "%s%.*s%s",
			re_prefix, core_len, &pre[pa_prefix_len], re_suffix);
		data = &data[re_prefix_len + core_len + re_suffix_len + 1];
	}
	return (const char **)newarr;
}

struct command
command_init(size_t item_count)
{
	struct command c = {0};
	c.len = 0;
	c.items = temp_alloc(item_count * sizeof(*c.items));
	c.size = item_count;
	return c;
}

void
command_append(struct command *c, ...)
{
	const char *s;
	va_list args;

	va_start(args, c);
	s = va_arg(args, char*);
	if (!s) return;

	while (s) {
		struct str *item = &c->items[c->len];
		size_t z = strlen(s);

		item->buf = temp_alloc(z + 1);
		strcpy(item->buf, s);
		item->len = z;

		c->len++;
		assert(c->len <= c->size);
		s = va_arg(args, char*);
	}

	va_end(args);
}

void
command_append_vec(struct command *c,
		const char *const *const vec, const size_t l)
{
	assert(c && vec && l > 0);
	for (size_t i = 0; i < l; i++) {
		command_append(c, vec[i], 0);
	}
}

void
command_append_vec0(struct command *c,
		const char *const *const vec)
{
	assert(c && vec);
	for (size_t i = 0; vec[i]; i++) {
		command_append(c, vec[i], 0);
	}
}

char *
command_string(struct command *c)
{
	size_t z = 0;

	z += c->items[0].len;
	for (size_t i = 1; i < c->len; i++) {
		struct str *item = &c->items[i];
		z += 1 + item->len;

		if (strchr(item->buf, ' ')) z += 2;
	}

	char *str = temp_alloc(z + 1);
	char *s = str;

	for (size_t i = 0; i < c->len; i++) {
		struct str *item = &c->items[i];
		if (strchr(item->buf, ' ')) {
			*s = '"';
			memcpy(&s[1], item->buf, item->len);
			s[item->len + 1] = '"';
			s = &s[item->len + 3];
		} else {
			memcpy(s, item->buf, item->len);
			s = &s[item->len + 1];
		}
		s[-1] = ' ';
	}
	s[-1] = '\0';

	return str;
}

char **
command_array(struct command *c)
{
	size_t z = 0;
	for (size_t i = 0; i < c->len; i++) {
		z += c->items[i].len + 1;
	}

	size_t array_pointers = c->len + 1;
	char **a = temp_alloc(array_pointers * sizeof(char*) + z);
	char *s = (char *)&a[c->len + 1];
	for (size_t i = 0; i < c->len; i++) {
		struct str item = c->items[i];
		strcpy(s, item.buf);
		a[i] = s;
		s = &s[item.len + 1];
	}

	return a;
}

bool
remove_file(const char *pathname)
{
#ifdef _WIN32
	return DeleteFile(pathname);
#else
	return unlink(pathname) != -1;
#endif
}

bool
remove_dir(const char *path)
{
#ifdef _WIN32
	/* https://www.codeproject.com/Articles/9089/Deleting-a-directory-along-with-sub-folders */
	HANDLE find;
	WIN32_FIND_DATA data;

	const char search[MAX_PATH] = {0};
	char buf[MAX_PATH] = {0};

	sprintf(search, "%s\\\*", path);

	find = FindFirstFile(search, &data);
	if (find == INVALID_HANDLE_VALUE) return false;

	for (;;) {
		if (!FindNextFile(find, &data)) {
			if (GetLastError() == ERROR_NO_MORE_FILES) break;
			FindClose(find);
			return false;
		}

		if (strcmp(data.cFileName, ".")
				&& strcmp(data.cFileName, "..")) {
			continue;
		}

		snprintf(buf, MAX_PATH, "%s\\%s", path, data.cFileName);
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (!remove_dir(buf)) {
				FindClose(find);
				return false;
			}
		} else if (!DeleteFile(buf)) {
			FindClose(find);
			return false;
		}
	}
	FindClose(find);
	return RemoveDirectory(path);
#else
	bool result = true;
	char buf[PATH_MAX] = {0};

	DIR *dir = opendir(path);
	if (!dir) return false;

	struct dirent *p;
	while (p = readdir(dir)) {
		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
			continue;
		}

		snprintf(buf, PATH_MAX, "%s/%s", path, p->d_name);

		struct stat st;
		if (stat(buf, &st)) {
			result = false;
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			result = result && remove_dir(buf);
		} else {
			result = result && !unlink(buf);
		}
	}
	closedir(dir);

	if (result) result = result && !rmdir(path);

	return result;
#endif
}

bool
create_dir(const char* pathname)
{
#ifdef _WIN32
	if (CreateDirectory(pathname, NULL)
			&& GetLastError() == ERROR_PATH_NOT_FOUND) {
		fprintf(stderr, "Err: create_dir %s: Path Not Found\n");
		return false;
	}
#else
	if (mkdir(pathname, 0755) && errno != EEXIST) {
		perror("mkdir");
		return false;
	}
#endif
	return true;
}

int /* ( -1:error | 0:false | 1:true ) */
target_needs_rebuild(const char *output, const char **inputs, size_t input_count)
{
#ifdef _WIN32
	BOOL success;
	HANDLE outfd = CreateFile(output, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (outfd == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			return 1;
		}
		fprintf(stderr, "Err: could not open file %s: %lu", output, GetLastError());
		return -1;
	}
	FILETIME outtime;
	success = GetFileTime(outfd, NULL, NULL, &outtime);
	CloseHandle(outfd);
	if (!success) {
		fprintf(stderr, "Err: Could not get time of %s: %lu", output, GetLastError());
		return -1;
	}

	for (size_t i = 0; i < input_count; i += 1) {
		const char *input = inputs[i];
		HANDLE infd = CreateFile(input, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (infd == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "Err: could not open file %s: %lu", input, GetLastError());
			return -1;
		}
		FILETIME intime;
		success = GetFileTime(infd, NULL, NULL, &intime);
		CloseHandle(infd);
		if (!success) {
			fprintf(stderr, "Err: Could not get time of %s: %lu", input, GetLastError());
			return -1;
		}

		if (CompareFileTime(&intime, &outtime) > 0)
			return 1;
	}

	return 0;

#else

	struct stat st = {0};
	if (stat(output, &st) < 0) {
		if (errno == ENOENT) return 1;
		fprintf(stderr, "Err: could not stat output %s: %s\n", output, strerror(errno));
		return -1;
	}
	int outtime = st.st_mtime;

	for (size_t i = 0; i < input_count; i += 1) {
		const char *input = inputs[i];
		if (stat(input, &st) < 0) {
			fprintf(stderr, "Err: could not stat input %s: %s\n", input, strerror(errno));
			return -1;
		}
		int intime = st.st_mtime;
		if (intime > outtime) return 1;
	}

	return 0;

#endif
}

Proc
proc_run(struct command *c)
{
#ifdef _WIN32

	/* https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output */
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(STARTUPINFO);
	/* NOTE: theoretically setting NULL to std handles
	 * should not be a problem
	 * https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
	 * TODO: check for errors in GetStdHandle */
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	/* printf("ZZZZZZZZZ --------- :%s\n", cmd); */
	/* FIXME: does this even work on win? Cannot test on linux */
	BOOL ok = CreateProcessA(
		NULL, command_string(c), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

	if (!ok) {
		fprintf(stderr, "Err: Could not create child process: %lu\n", GetLastError());
		return INVALID_PROC;
	}

	CloseHandle(pi.hThread);
	return pi.hProcess;

#else

	pid_t pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Err: Could not fork process: %s\n", strerror(errno));
		return INVALID_PROC;
	}

	if (pid > 0) return pid;

	char **cmd = command_array(c);
	if (execvp(cmd[0], cmd) < 0) {
		fprintf(stderr, "Err: Could not exec process: %s\n", strerror(errno));
		exit(1);
	}

	assert(0 && "unreachable");
#endif /* _WIN32 */
}

bool
proc_wait(Proc proc)
{
	if (proc == INVALID_PROC) return 0;

#ifdef _WIN32

	DWORD result = WaitForSingleObject(proc, INFINITE);
	if (result == WAIT_FAILED) {
		fprintf(stderr, "Err: could not wait on process: %lu\n", GetLastError());
		return 0;
	}

	DWORD exit_status;
	if (!GetExitCodeProcess(proc, &exit_status)) {
		fprintf(stderr, "Err: could not get process exit code: %lu\n",
			GetLastError());
		return 0;
	}

	if (exit_status != 0) {
		fprintf(stderr, "Err: command exited with exit code %lu\n",
			exit_status);
		return 0;
	}

	CloseHandle(proc);
	return 1;

#else

	for (;;) {
		int wstatus = 0;
		if (waitpid(proc, &wstatus, 0) < 0) {
			fprintf(stderr, "Err: could not wait on command (pid %d): %s\n",
				proc, strerror(errno));
			return 0;
		}

		if (WIFEXITED(wstatus)) {
			int exit_status = WEXITSTATUS(wstatus);
			if (exit_status != 0) {
				fprintf(stderr, "Err: command exited with exit code %d\n",
					exit_status);
				return 0;
			}
			break;
		}

		if (WIFSIGNALED(wstatus)) {
			fprintf(stderr, "Err: command was terminated by %s\n",
				strsignal(WTERMSIG(wstatus)));
			return 0;
		}
	}
	return 1;

#endif
}

#endif /* BUILD_H */
