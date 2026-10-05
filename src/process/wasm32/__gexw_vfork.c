/* wasm32 (gex): vfork without a second stack.
 *
 * gex cannot fork a wasm32 program (its wasm frames cannot be copied or
 * resumed elsewhere), so vfork() is a macro (unistd.h) around a setjmp in
 * the caller's frame: the "child" runs on in the parent's own thread, as
 * vfork's borrowed-stack child does, and its execve becomes a posix_spawn
 * (a new process with the fd table, working directory, umask and signal
 * state the child has set up by then). Then the parent's own state is put
 * back and the setjmp returns again, with the new pid, as vfork returns in
 * the parent. An _exit in the child spawns this program again just to exit
 * with that status (so the parent can wait for it). One vfork at a time;
 * the child must not return from the function that called vfork (as with
 * any vfork).
 */
#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include "syscall.h"

#define VF_FDS 256
#define VF_SIGS 65

static struct {
	int active;
	pid_t pid;
	jmp_buf jb;
	pid_t result;
	int err;
	unsigned char open[VF_FDS], cloexec[VF_FDS];
	int backup[VF_FDS];
	int cwd;
	mode_t umask;
	sigset_t mask;
	struct sigaction sa[VF_SIGS];
} vf;

extern char **__environ;

struct __jmp_buf_tag *__gexw_vfork_jb(void)
{
	return vf.jb;
}

/* In the child (this thread, until its exec or exit)? Not in a process
 * posix_spawn made, which has a copy of this state. */
int __gexw_vfork_active(void)
{
	return vf.active && __syscall(SYS_getpid) == vf.pid;
}

pid_t __gexw_vfork_child(void)
{
	int fd, s;
	if (vf.active) {
		errno = EAGAIN;
		return -1;
	}
	vf.pid = __syscall(SYS_getpid);
	for (fd = 0; fd < VF_FDS; fd++) {
		int fl = __syscall(SYS_fcntl64, fd, F_GETFD);
		vf.open[fd] = fl >= 0;
		if (fl < 0) continue;
		vf.cloexec[fd] = fl & FD_CLOEXEC;
		vf.backup[fd] = __syscall(SYS_fcntl64, fd, F_DUPFD_CLOEXEC, VF_FDS);
	}
	fd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	vf.cwd = fd >= 0 ? __syscall(SYS_fcntl64, fd, F_DUPFD_CLOEXEC, VF_FDS) : -1;
	if (fd >= 0) __syscall(SYS_close, fd);
	vf.umask = umask(0);
	umask(vf.umask);
	sigprocmask(SIG_SETMASK, 0, &vf.mask);
	for (s = 1; s < VF_SIGS; s++) sigaction(s, 0, &vf.sa[s]);
	vf.active = 1;
	return 0;
}

/* The parent's state back, as before the vfork. */
static void restore(void)
{
	int fd, s;
	for (fd = 0; fd < VF_FDS; fd++) {
		if (vf.open[fd] && vf.backup[fd] >= 0) {
			__syscall(SYS_dup2, vf.backup[fd], fd);
			__syscall(SYS_fcntl64, fd, F_SETFD, vf.cloexec[fd] ? FD_CLOEXEC : 0);
			__syscall(SYS_close, vf.backup[fd]);
		} else if (!vf.open[fd]) {
			__syscall(SYS_close, fd);
		}
	}
	if (vf.cwd >= 0) {
		fchdir(vf.cwd);
		__syscall(SYS_close, vf.cwd);
	}
	umask(vf.umask);
	for (s = 1; s < VF_SIGS; s++) sigaction(s, &vf.sa[s], 0);
	sigprocmask(SIG_SETMASK, &vf.mask, 0);
	vf.active = 0;
}

static _Noreturn void back_to_parent(pid_t pid, int err)
{
	restore();
	vf.result = pid;
	vf.err = err;
	longjmp(vf.jb, 1);
}

pid_t __gexw_vfork_parent(void)
{
	if (vf.result < 0) errno = vf.err;
	return vf.result;
}

/* The child's execve: the program, spawned with the child's state. Returns
 * (with errno) only if that cannot be done, as execve would. */
int __gexw_vfork_execve(const char *path, char *const argv[], char *const envp[])
{
	pid_t pid;
	int e = posix_spawn(&pid, path, 0, 0, argv, envp);
	if (e) {
		errno = e;
		return -1;
	}
	back_to_parent(pid, 0);
}

/* The child's _exit: a child process that exits with that status. */
_Noreturn void __gexw_vfork_exit(int ec)
{
	char var[32], *argv[2] = { "vfork-exit", 0 }, **env;
	size_t n = 0;
	pid_t pid;
	while (__environ && __environ[n]) n++;
	env = calloc(n + 2, sizeof *env);
	snprintf(var, sizeof var, "__GEXW_VFORK_EXIT=%d", ec & 0xff);
	if (env) {
		memcpy(env, __environ, n * sizeof *env);
		env[n] = var;
	}
	if (!env || posix_spawn(&pid, "/proc/self/exe", 0, 0, argv, env)) {
		free(env);
		back_to_parent(-1, ECHILD);
	}
	free(env);
	back_to_parent(pid, 0);
}

/* That child: exit at once, before main. */
__attribute__((constructor)) static void vfork_exit_child(void)
{
	char *e = getenv("__GEXW_VFORK_EXIT");
	if (e) _Exit(atoi(e));
}
