#ifndef BUILD_H
#define BUILD_H

#if defined(PLATFORM_WINDOWS) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#	define PLATFORM_WINDOWS 1
#else
#	define PLATFORM_WINDOWS 0
#endif

#if !PLATFORM_WINDOWS
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

#if PLATFORM_WINDOWS
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

#if PLATFORM_WINDOWS
	typedef HANDLE Proc;
#	define INVALID_PROC INVALID_HANDLE_VALUE
#	define SEP '\\'
#else
	typedef int Proc;
#	define INVALID_PROC (-1)
#	define SEP '/'
#endif

#ifndef SCRATCH_SIZE
#define SCRATCH_SIZE 0x2000
#endif
struct scratch {
	char buf[SCRATCH_SIZE];
	size_t len;
} scratch;

#ifndef ARENA_SIZE
#define ARENA_SIZE 0x4000
#endif
char arena_buffer[ARENA_SIZE] = {0};
struct arena {
	size_t count;
	size_t save;
	size_t size;
	char *buf;
} arena = { .size = ARENA_SIZE, .buf = arena_buffer };

struct vheader {
	size_t size;
	size_t capacity;
	char buf[];
};

void *
xcalloc(size_t n, size_t s)
{
	void *p = calloc(n, s);
	if (!p && s) {
		abort();
	}
	return p;
}

void *
xrealloc(void *p, size_t s)
{
	p = realloc(p, s);
	if (!p && s) {
		abort();
	}
	return p;
}

int
xvfprintf(FILE *restrict f, const char *restrict fmt, va_list ap)
{
	int n = vfprintf(f, fmt, ap);
	if (f != stderr && n < 0) {
		perror("fprintf");
		exit(255);
	}
	return n;
}

int
xfprintf(FILE *restrict f, const char *restrict fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = xvfprintf(f, fmt, ap);
	va_end(ap);
	return n;
}

void *
arena_realloc(void *p, size_t size)
{
	assert(size < ARENA_SIZE);

	size_t fatsize = size + sizeof(size_t);
	while (arena.count + fatsize > arena.size) {
		arena.size *= 2;
		arena.buf = xrealloc(arena.buf, arena.size);
	}

	size_t *fatpointer = (size_t*)&arena.buf[arena.count];
	void *pointer = fatpointer + 1;
	memset(fatpointer, 0, fatsize);
	*fatpointer = size;

	if (p) {
		size_t oldsize = ((size_t*)p)[-1];
		memcpy(pointer, p, (size < oldsize ? size : oldsize));
	}
	arena.count += fatsize;

	return pointer;
}

void *
arena_alloc(size_t size)
{
	return arena_realloc(NULL, size);
}

char *
arena_strndup(const char *str, const size_t len)
{
	char *dup = arena_alloc(len + 1);
	memcpy(dup, str, len);
	return dup;
}

void
scratch_add_char(const char ch)
{
	if (scratch.len + 1 > SCRATCH_SIZE - 1) {
		xfprintf(stderr, "Scratch buffer size (%d) exceeded\n", SCRATCH_SIZE - 1);
		abort();
	}
	char *s = &scratch.buf[scratch.len];
	*s = ch;
	scratch.len++;
}

void
scratch_add_str_len(const char *restrict str, const size_t len)
{
	if (len + scratch.len > SCRATCH_SIZE - 1) {
		xfprintf(stderr, "Scratch buffer size (%d) exceeded\n", SCRATCH_SIZE - 1);
		abort();
	}
	memcpy(&scratch.buf[scratch.len], str, len);
	scratch.len += len;
}

void
scratch_add_str(const char *restrict str)
{
	scratch_add_str_len(str, strlen(str));
}

void
scratch_add_fmt(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t available = SCRATCH_SIZE - scratch.len;

	size_t needed = (size_t)vsnprintf(
		&scratch.buf[scratch.len], available, fmt, args);
	if (needed > available - 1) {
		xfprintf(stderr, "Scratch buffer size (%d) exceeded\n", SCRATCH_SIZE - 1);
		abort();
	}
	va_end(args);
	scratch.len += needed;
}

char *
scratch_tostring(void)
{
	scratch_add_char('\0');
	return scratch.buf;
}

static struct vheader *
vec_create(size_t capacity)
{
	assert(capacity < SIZE_MAX);
	struct vheader *header =
		arena_alloc(sizeof(*header) + sizeof(char*) * capacity);
	header->capacity = capacity;
	return header;
}

size_t
vec_size(char **vec)
{
	if (!vec) return 0;
	const struct vheader *header = ((struct vheader*)vec) - 1;
	return header->size;
}

static char **
vec_expand(char **vec)
{
	struct vheader *header;

	if (vec) {
		header = ((struct vheader*)vec) - 1;
	} else {
		header = vec_create(8);
	}

	if (header->size == header->capacity) {
		struct vheader *newvec =
			vec_create(header->capacity << 1U);
		size_t z = sizeof(char*) * header->size + sizeof(*header);
		memcpy(newvec, header, z);
		newvec->capacity = header->capacity << 1U;
	}
	header->size++;
	return (char **)&(header[1]);
}

void
vec_add(char ***vec, char *value)
{
	*vec = vec_expand(*vec);
	(*vec)[vec_size(*vec) - 1] = value;
}

void
vec_add_vec(char ***vec, char **vec2)
{
	assert(vec2);
	struct vheader *head2 = ((struct vheader*)vec2) - 1;
	for (size_t i = 0; i < head2->size; i++) {
		vec_add(vec, vec2[i]);
	}
}

void
vec_add_many(char ***vec, ...)
{
	assert(vec);
	char *s;
	va_list args;

	va_start(args, vec);
	s = va_arg(args, char*);
	if (!s) return;

	while (s) {
		vec_add(vec, s);
		s = va_arg(args, char*);
	}

	va_end(args);
}

/* Check if vec is null-terminated. If not, add empty pointer. */
char **
vec_array(char ***vec)
{
	assert(vec);
	if ((*vec)[vec_size(*vec) - 1] != NULL) {
		vec_add(vec, NULL);
	}
	return *vec;
}

static void
string_fromvec_escape(bool escaping_needed, char *s)
{
	size_t l = strlen(s);
	if (escaping_needed) scratch_add_char('"');
	for (size_t i = 0; i < l; i++) {
		if (s[i] == '"' || s[i] == '\\') scratch_add_char('\\');
		scratch_add_char(s[i]);
	}
	if (escaping_needed) scratch_add_char('"');

}

char *
string_fromvec(char **vec)
{
	assert(vec);

	scratch.len = 0;

	bool escaping_needed = strpbrk(vec[0], "\t\v ");
	string_fromvec_escape(escaping_needed, vec[0]);

	for (size_t i = 1; i < vec_size(vec); i++) {
		escaping_needed = strpbrk(vec[i], "\t\v ");
		scratch_add_char(' ');
		string_fromvec_escape(escaping_needed, vec[i]);
	}
	return scratch_tostring();
}

const char **
string_substitute_vec(char **vec, const char *pattern, const char *replacement)
{
	assert(vec && pattern && replacement);
	struct vheader *head = (struct vheader*)vec - 1;

	char *pa = strchr(pattern, '*');
	char *re = strchr(replacement, '*');
	assert(pa && re);

	scratch.len = 0;

	size_t pa_prefix_len = (size_t)(pa - pattern);
	char *pa_prefix = arena_strndup(pattern, pa_prefix_len);

	size_t pa_suffix_len = strlen(pa + 1);
	char *pa_suffix = arena_strndup(pa + 1, pa_suffix_len);

	size_t re_prefix_len = (size_t)(re - replacement);
	char *re_prefix = arena_strndup(replacement, re_prefix_len);

	size_t re_suffix_len = strlen(re + 1);
	char *re_suffix = arena_strndup(re + 1, re_suffix_len);

	char **mapped = NULL;
	for (size_t i = 0; i < head->size; i++) {
		char *pre = strstr(vec[i], pa_prefix);
		if (pre != vec[i]) {
			vec_add(&mapped, vec[i]);
			continue;
		}
		char *suf = strstr(vec[i], pa_suffix);
		if (strlen(suf) != pa_suffix_len) {
			vec_add(&mapped, vec[i]);
			continue;
		}

		size_t core_len = (size_t)(suf - pre) - pa_prefix_len;

		scratch.len = 0;
		scratch_add_str_len(re_prefix, re_prefix_len);
		scratch_add_str_len(&pre[pa_prefix_len], core_len);
		scratch_add_str_len(re_suffix, re_suffix_len);
		vec_add(&mapped, scratch_tostring());
	}
	return (const char **)mapped;
}

bool
remove_file(const char *pathname)
{
#if PLATFORM_WINDOWS
	return DeleteFile(pathname);
#else
	return unlink(pathname) != -1;
#endif
}

bool
remove_dir(const char *path)
{
#if PLATFORM_WINDOWS
	/* https://www.codeproject.com/Articles/9089/Deleting-a-directory-along-with-sub-folders */
	HANDLE find;
	WIN32_FIND_DATA data;

	char search[MAX_PATH] = {0};
	char buf[MAX_PATH] = {0};

	sprintf(search, "%s\\%c", path, '*');

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
	while ((p = readdir(dir))) {
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
#if PLATFORM_WINDOWS
	if (CreateDirectory(pathname, NULL)
			&& GetLastError() == ERROR_PATH_NOT_FOUND) {
		xfprintf(stderr, "Err: create_dir %s: Path Not Found\n");
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
#if PLATFORM_WINDOWS
	BOOL success;
	HANDLE outfd = CreateFile(output, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (outfd == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			return 1;
		}
		xfprintf(stderr, "Err: could not open file %s: %lu\n",
			output, GetLastError());
		return -1;
	}
	FILETIME outtime;
	success = GetFileTime(outfd, NULL, NULL, &outtime);
	CloseHandle(outfd);
	if (!success) {
		xfprintf(stderr, "Err: Could not get time of %s: %lu\n",
			output, GetLastError());
		return -1;
	}

	for (size_t i = 0; i < input_count; i += 1) {
		const char *input = inputs[i];
		HANDLE infd = CreateFile(input, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (infd == INVALID_HANDLE_VALUE) {
			xfprintf(stderr, "Err: could not open file %s: %lu\n",
				input, GetLastError());
			return -1;
		}
		FILETIME intime;
		success = GetFileTime(infd, NULL, NULL, &intime);
		CloseHandle(infd);
		if (!success) {
			xprintf(stderr, "Err: Could not get time of %s: %lu\n",
				input, GetLastError());
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
		xfprintf(stderr, "Err: could not stat output %s: %s\n",
			output, strerror(errno));
		return -1;
	}
	int64_t outtime = st.st_mtime;

	for (size_t i = 0; i < input_count; i += 1) {
		const char *input = inputs[i];
		if (stat(input, &st) < 0) {
			xfprintf(stderr, "Err: could not stat input %s: %s\n",
				input, strerror(errno));
			return -1;
		}
		int64_t intime = st.st_mtime;
		if (intime > outtime) return 1;
	}

	return 0;

#endif
}

Proc
proc_run(char **vec)
{
	assert(vec);
#if PLATFORM_WINDOWS

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
		NULL, string_fromvec(vec), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

	if (!ok) {
		xfprintf(stderr, "Err: Could not create child process: %lu\n",
			GetLastError());
		return INVALID_PROC;
	}

	CloseHandle(pi.hThread);
	return pi.hProcess;

#else

	pid_t pid = fork();
	if (pid < 0) {
		xfprintf(stderr, "Err: Could not fork process: %s\n", strerror(errno));
		return INVALID_PROC;
	}

	if (pid > 0) return pid;

	char **cmd = vec_array(&vec);
	if (execvp(cmd[0], cmd) < 0) {
		xfprintf(stderr, "Err: Could not exec process: %s\n", strerror(errno));
		exit(1);
	}

	assert(0); // Unreachable

#endif /* PLATFORM_WINDOWS */
}

bool
proc_wait(Proc proc)
{
	if (proc == INVALID_PROC) return 0;

#if PLATFORM_WINDOWS

	DWORD result = WaitForSingleObject(proc, INFINITE);
	if (result == WAIT_FAILED) {
		xfprintf(stderr, "Err: could not wait on process: %lu\n", GetLastError());
		return 0;
	}

	DWORD exit_status;
	if (!GetExitCodeProcess(proc, &exit_status)) {
		xfprintf(stderr, "Err: could not get process exit code: %lu\n",
			GetLastError());
		return 0;
	}

	if (exit_status != 0) {
		xfprintf(stderr, "Err: command exited with exit code %lu\n",
			exit_status);
		return 0;
	}

	CloseHandle(proc);
	return 1;

#else

	for (;;) {
		int wstatus = 0;
		if (waitpid(proc, &wstatus, 0) < 0) {
			xfprintf(stderr, "Err: could not wait on command (pid %d): %s\n",
				proc, strerror(errno));
			return 0;
		}

		if (WIFEXITED(wstatus)) {
			int exit_status = WEXITSTATUS(wstatus);
			if (exit_status != 0) {
				xfprintf(stderr, "Err: command exited with exit code %d\n",
					exit_status);
				return 0;
			}
			break;
		}

		if (WIFSIGNALED(wstatus)) {
			xfprintf(stderr, "Err: command was terminated by %s\n",
				strsignal(WTERMSIG(wstatus)));
			return 0;
		}
	}
	return 1;

#endif
}

#endif /* BUILD_H */
