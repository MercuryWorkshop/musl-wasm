/* wasm32 (gex): fork by unwinding and rewinding the wasm stack.
 *
 * gex cannot copy a wasm call stack (its frames are the engine's, not
 * linear memory), so a program that links fork is instrumented with
 * binaryen's asyncify after the link (tools/wine-wasm/wcc): every function
 * that can reach the gex.fork_point import can save its locals and call
 * position to a buffer in linear memory and later resume from it.
 *
 * Every thread's entry runs its function under __gex_fork_run (the
 * driver, not instrumented). A fork:
 *   1. __gex_fork maps a record (asyncify's {cur, end} buffer, the C stack
 *      pointer) and calls gex.fork_point, where the
 *      host starts the unwind: every frame up to the driver saves itself
 *      and returns;
 *   2. the driver stops the unwind, notes its function in the record and
 *      makes the child: a clone(SIGCHLD)
 *      (a new process, a copy of the address space, record included)
 *      whose thread starts in __gex_fork_child;
 *   3. both start the rewind and call the driver's function again: every
 *      frame is restored and calls its callee again, down to fork_point,
 *      where the host stops the rewind; __gex_fork returns the pid in the
 *      parent and 0 in the child.
 * As with fork on Linux, only the calling thread exists in the child.
 */
#define _GNU_SOURCE
#include <signal.h>
#include <sched.h>
#include <sys/mman.h>
#include "pthread_impl.h"
#include "syscall.h"

__attribute__((__import_module__("gex"), __import_name__("fork_point")))
void __gex_fork_point(void *);
/* asyncify's exports, on the calling instance: 0 state (0 normal,
 * 1 unwinding, 2 rewinding), 1 start_unwind(data), 2 stop_unwind,
 * 3 start_rewind(data), 4 stop_rewind. */
__attribute__((__import_module__("gex"), __import_name__("asyncify")))
int __gex_asyncify(int, void *);

unsigned long __gex_get_sp(void);
void __gex_set_sp(unsigned long);
int __clone(int (*)(void *), void *, int, void *, ...);

#define REC_SIZE (1 << 20)
#define CHILD_STACK 8192

struct driver {
	int (*fn)(void *);
	void *arg;
	unsigned long sp;
};

struct rec {
	/* asyncify's data: where the next frame goes, and the end */
	char *cur, *end;
	long result;
	unsigned long sp;
	struct driver drv;
};

static struct rec *volatile fork_rec;

hidden int __gex_fork_child(void *);

/* Run fn(arg) until it returns; an unwind for a fork stops here. Not
 * instrumented (wcc's asyncify removelist), and with no frame of its own
 * in linear memory: the C stack pointer is reset under it. */
hidden __attribute__((noinline)) int __gex_fork_drive(int (*fn)(void *), void *arg, unsigned long sp)
{
	for (;;) {
		__gex_set_sp(sp);
		int r = fn(arg);
		if (__gex_asyncify(0, 0) != 1) return r;
		__gex_asyncify(2, 0);
		/* this driver's function is what the child calls again */
		struct rec *f = fork_rec;
		f->drv.fn = fn;
		f->drv.arg = arg;
		f->drv.sp = sp;
		f->result = __clone(__gex_fork_child, (char *)f + REC_SIZE, SIGCHLD, f);
		__gex_asyncify(3, f);
	}
}

/* A thread's function (the program's start, a pthread's), under the
 * driver. */
hidden int __gex_fork_run(int (*fn)(void *), void *arg)
{
	return __gex_fork_drive(fn, arg, __gex_get_sp());
}

/* The child's thread (a clone's, on the record's stack, with the forking
 * thread's thread pointer and TLS): the rewind. */
hidden int __gex_fork_child(void *p)
{
	struct rec *f = p;
	f->result = 0;
	__gex_asyncify(3, f);
	return __gex_fork_drive(f->drv.fn, f->drv.arg, f->drv.sp);
}

long __gex_fork(void)
{
	struct rec *f = (void *)__syscall(SYS_mmap2, 0, REC_SIZE,
		PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if ((unsigned long)f > -4096UL) return (long)f;
	f->cur = (char *)(f + 1);
	f->end = (char *)f + REC_SIZE - CHILD_STACK;
	f->sp = __gex_get_sp();
	fork_rec = f;
	__gex_fork_point(f);
	/* rewound, in the parent or the child */
	f = fork_rec;
	__gex_set_sp(f->sp);
	long r = f->result;
	fork_rec = 0;
	__syscall(SYS_munmap, f, REC_SIZE);
	return r;
}
