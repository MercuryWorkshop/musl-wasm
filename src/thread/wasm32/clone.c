/* wasm32 (gex): a clone starts the child at a fixed guest address, the
 * image's thread entry (AT_ENTRY + 16), which gex binds to
 * __gex_thread_start. The child's function, argument and thread pointer
 * travel on top of its stack; the entry takes them from there.
 *
 * A clone without CLONE_THREAD (posix_spawn's CLONE_VM|CLONE_VFORK, or a
 * fork) is a new process under gex: a copy of the address space whose
 * thread starts at the same entry. */
#define _GNU_SOURCE
#include <stdarg.h>
#include <sched.h>
#include <sys/auxv.h>
#include "pthread_impl.h"

unsigned long __gex_get_ctx(void);

int __clone(int (*func)(void *), void *stack, int flags, void *arg, ...)
{
	va_list ap;
	pid_t *ptid, *ctid;
	void *tls;

	va_start(ap, arg);
	ptid = va_arg(ap, pid_t *);
	tls = va_arg(ap, void *);
	ctid = va_arg(ap, pid_t *);
	va_end(ap);

	unsigned long *sp = (unsigned long *)((uintptr_t)stack & -16);
	sp -= 4;
	sp[0] = (unsigned long)func;
	sp[1] = (unsigned long)arg;
	/* The child's module instance starts with fresh globals: it gets a
	 * thread pointer either way, and a fresh TLS block only for
	 * CLONE_SETTLS (otherwise it shares the caller's, as on Linux). */
	sp[2] = (flags & CLONE_SETTLS) ? (unsigned long)tls : (unsigned long)__pthread_self();
	sp[3] = !!(flags & CLONE_SETTLS);

	/* The i386 kernel takes a struct user_desc for CLONE_SETTLS; the
	 * thread pointer is the entry's to set, so the kernel gets none. */
	flags &= ~CLONE_SETTLS;

	volatile unsigned long long *x = (volatile unsigned long long *)__gex_get_ctx();
	unsigned long long pc_save = x[32];
	x[32] = getauxval(AT_ENTRY) + 16;
	long r = __syscall(SYS_clone, flags, sp, ptid, 0, ctid);
	x[32] = pc_save;
	return __syscall_ret(r);
}
