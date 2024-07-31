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
#	include <sys/wait.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

#ifdef _WIN32
#	if defined(__GNUC__)
#		define CC "gcc"
#	elif defined(__clang__)
#		define CC "clang"
#	elif defined(_MSC_VER)
#		define CC "cl"
#	endif
#elif defined(_USE_MINGW)
#	define CC "x86_64-w64-mingw32-cc"
#else
#	define CC "cc"
#endif

#ifdef _WIN32
	typedef HANDLE Proc;
#	define INVALID_PROC INVALID_HANDLE_VALUE
#	define SEP '\\'
#	define PATH_MAX MAX_PATH
#else
	typedef int Proc;
#	define INVALID_PROC (-1)
#	define SEP '/'
#endif

typedef uint8_t   u8;
typedef int8_t    i8;
typedef uint16_t  u16;
typedef int16_t   i16;
typedef uint32_t  u32;
typedef int32_t   i32;
typedef uint64_t  u64;
typedef int64_t   i64;
typedef uintptr_t uptr;
typedef ptrdiff_t ptrdiff;
typedef size_t    usize;

#define size(a)  sizeof(a)
#define count(a) (size(a)/size(*(a)))
#define len(s)   (count(s) - 1)

#ifndef TEMP_MAX
#define TEMP_MAX 8 * 1024
#endif
char temporary[TEMP_MAX] = {0};
usize temporary_count = 0;

struct str {
	usize len;
	char *buf;
};

struct path {
	char buf[PATH_MAX];
	usize len;
};

struct object {
	usize input_count;
	int type;
	const char *const output;
	const char *const *const inputs;
};

struct command {
	usize size;
	usize len;
	struct str *items;
};

#define str(s) (struct str){ .len = len(s), .buf = s }

void *
temp_alloc(usize size)
{
	assert(size < TEMP_MAX);

	if (temporary_count + size > TEMP_MAX) {
		temporary_count = 0;
	}

	usize count = temporary_count;
	memset(&temporary[count], 0, size);
	temporary_count += size;

	return &temporary[count];
}

char *
temp_fmt(const char *fmt, ...) {
	char *str;
	usize len;
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

struct command
command_init(usize item_count)
{
	struct command c = {0};
	c.len = 0;
	c.items = temp_alloc(item_count * size(*c.items));
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
		usize z = strlen(s);

		item->buf = temp_alloc(z + 1);
		strcpy(item->buf, s);
		item->len = z;

		c->len++;
		assert(c->len <= c->size);
		s = va_arg(args, char*);
	}

	va_end(args);
}

char *
command_string(struct command *c)
{
	usize z = 0;

	z += c->items[0].len;
	for (usize i = 1; i < c->len; i++) {
		struct str *item = &c->items[i];
		z += 1 + item->len;

		if (strchr(item->buf, ' ')) z += 2;
	}

	char *str = temp_alloc(z + 1);
	char *s = str;

	for (usize i = 0; i < c->len; i++) {
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
	usize z = 0;
	for (usize i = 0; i < c->len; i++) {
		z += c->items[i].len + 1;
	}

	usize array_pointers = c->len + 1;
	char **a = temp_alloc(array_pointers * sizeof(char*) + z);
	char *s = (char *)&a[c->len + 1];
	for (usize i = 0; i < c->len; i++) {
		struct str item = c->items[i];
		strcpy(s, item.buf);
		a[i] = s;
		s = &s[item.len + 1];
	}

	return a;
}

char *
string_replace_last_n_chars(const char *const s,
		const char *const r, const usize n)
{
	usize l = strlen(s);
	assert(n < l);
	char *o = temp_alloc(l + 1);
	memcpy(o, s, l - n);
	memcpy(&o[l - n], r, n);
	return o;
}

#define errorln(...) errorln0(__VA_ARGS__ __VA_OPT__(,) NULL)
void
errorln0(const char *const s, ...)
{
	if (!s) goto done;

	fputs(s, stderr);

	va_list args;
	va_start(args, s);

	char *a = va_arg(args, char*);
	do {
		fputc(' ', stderr);
		fputs(a, stderr);
		a = va_arg(args, char*);
	} while (a);

	va_end(args);
done:
	fputc('\n', stderr);
}

void
path_append(struct path path, const struct str *const str)
{
	/* FIXME: should probably be a regular error */
	assert(str->len + path.len < PATH_MAX && "resulting path is too big");

	char *last = &path.buf[path.len - 1];
	const char *src = str->buf;

	if (*last != SEP && *str->buf != SEP) {
		path.buf[path.len] = SEP;
		path.len += 1;
	} else if (*last == SEP && *str->buf == SEP) {
		/* instead of this should just append & clean up path */
		src = &str->buf[1];
	}

	char *dst = &path.buf[path.len];

	strcpy(dst, src);
	path.len += str->len;
}

struct object
object_init(int type, const char *const output, const char *const *const inputs)
{
	assert(output && inputs);

	usize count = 0;
	while (inputs[count]) count++;

	return (struct object){
		.type = type, .output = output,
		.input_count = count, .inputs = inputs,
	};
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
	free(cmd);

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

int /* ( -1:error | 0:false | 1:true ) */
object_needs_rebuild(struct object *object)
{
	const char *const output = object->output;
	const char *const *const inputs = object->inputs;
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

	for (usize i = 0; inputs[i]; i += 1) {
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

	for (usize i = 0; inputs[i]; i += 1) {
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
