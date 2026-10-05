#include <unistd.h>
#include "syscall.h"

int __gexw_vfork_active(void);
int __gexw_vfork_execve(const char *, char *const [], char *const []);

int execve(const char *path, char *const argv[], char *const envp[])
{
	/* in a vfork child (wasm32: the parent's own thread, __gexw_vfork.c) */
	if (__gexw_vfork_active()) return __gexw_vfork_execve(path, argv, envp);
	return syscall(SYS_execve, path, argv, envp);
}
