#ifndef _WIN32
#	define _POSIX_C_SOURCE 200809L
#endif
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
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

#define countof(a)  (sizeof(a)/sizeof(*(a)))
#define lengthof(s) (countof(s) - 1)

#define foreach(o, a) \
	for (usize i = 0; i < countof(a); i += 1) { \
		o = a[i];

typedef struct {
	i8 ok;
	i8 val;
} i8Result;

#ifdef _WIN32
#	if defined(__GNUC__)
#		define CC "cc"
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
#else
	typedef int Proc;
#	define INVALID_PROC (-1)
#endif

char *
vec2str(char *vec[])
{
	char *string = malloc(1);
	usize sz = 0, i = 0;
	do {
		char *arg = vec[i];
		usize len = strlen(arg);

		i8 has_specials = strchr(arg, ' ') || strchr(arg, '"');

		char *tmp = realloc(string,
			(sz + len + (has_specials ? 3 : 1))*sizeof(char));
		assert(tmp);
		string = tmp;

		if (has_specials) {
			string[sz] = '\'';
			sz += 1;
		}
		strncpy(&string[sz], arg, len);
		if (has_specials) {
			string[sz + len] = '\'';
			sz += 1;
		}
		string[sz + len] = ' ';
		sz += len + 1;

		i += 1;
	} while (vec[i]);
	string[sz] = '\0';

	return string;
}

Proc
create_process(char *command[])
{
#ifdef _WIN32

	/* https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output */
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(STARTUPINFO);
	// NOTE: theoretically setting NULL to std handles should not be a problem
	// https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
	// TODO: check for errors in GetStdHandle
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	char *cmd = vec2str(command);
	/* printf("ZZZZZZZZZ --------- :%s\n", cmd); */
	/* FIXME: does this even work on win? Cannot test on linux */
	BOOL ok = CreateProcessA(
		NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
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

	if (execvp(command[0], command) < 0) {
		fprintf(stderr, "Err: Could not exec process: %s\n", strerror(errno));
		exit(1);
	}

	assert(0 && "unreachable");
#endif /* _WIN32 */
}

i8
wait_for_process(Proc proc)
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

i8Result
needs_rebuild(const char *output, const char **inputs)
{
#ifdef _WIN32
	BOOL success;
	HANDLE outfd = CreateFile(output, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	if (outfd == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			return (i8Result){ .ok = 1, .val = 1 };
		}
		fprintf(stderr, "Err: could not open file %s: %lu", output, GetLastError());
		return (i8Result){ .ok = 0 };
	}
	FILETIME outtime;
	success = GetFileTime(outfd, NULL, NULL, &outtime);
	CloseHandle(outfd);
	if (!success) {
		fprintf(stderr, "Err: Could not get time of %s: %lu", output, GetLastError());
		return (i8Result){ .ok = 0 };
	}

	for (usize i = 0; inputs[i]; i += 1) {
		const char *input = inputs[i];
		HANDLE infd = CreateFile(input, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (infd == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "Err: could not open file %s: %lu", input, GetLastError());
			return (i8Result){ .ok = 0 };
		}
		FILETIME intime;
		success = GetFileTime(infd, NULL, NULL, &intime);
		CloseHandle(infd);
		if (!success) {
			fprintf(stderr, "Err: Could not get time of %s: %lu", input, GetLastError());
			return (i8Result){ .ok = 0 };
		}

		if (CompareFileTime(&intime, &outtime) > 0)
			return (i8Result){ .ok = 1, .val = 1 };
	}

	return (i8Result){ .ok = 1, .val = 0 };

#else

	struct stat st = {0};
	if (stat(output, &st) < 0) {
		if (errno == ENOENT) return (i8Result){ .ok = 1, .val = 1 };
		fprintf(stderr, "Err: could not stat %s: %s\n", output, strerror(errno));
		return (i8Result){ .ok = 0 };
	}
	int outtime = st.st_mtime;

	for (usize i = 0; inputs[i]; i += 1) {
		const char *input = inputs[i];
		if (stat(input, &st) < 0) {
			fprintf(stderr, "Err: could not stat %s: %s\n", input, strerror(errno));
			return (i8Result){ .ok = 0 };
		}
		int intime = st.st_mtime;
		if (intime > outtime) return (i8Result){ .ok = 1, .val = 1 };
	}

	return (i8Result){ .ok = 1, .val = 0 };

#endif
}
