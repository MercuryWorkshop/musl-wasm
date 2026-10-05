/* wasm32 (gex): the program's entry. gex enters __gex_start (gex_thread.s),
 * which sets up the thread's globals and C stack and calls this with the
 * initial stack pointer, as any _start_c gets it. */
#include <features.h>
#include "libc.h"

/* clang renames main on wasm: main(void) is __original_main and
 * main(int, char **) is __main_argc_argv. */
/* (__main_argc_argv referenced strongly, so a main in an archive member is
 * pulled in; libc's own weak definition, for main(void), calls
 * __original_main: src/env/wasm32/__main_argc_argv.c) */
int __main_argc_argv(int argc, char **argv);
static int main_shim(int argc, char **argv, char **envp) { return __main_argc_argv(argc, argv); }
weak void _fini();
/* wasm-ld gathers the program's constructors into __wasm_call_ctors (a
 * wasm module has no .init_array to walk); libc_start_init calls _init
 * after libc's own setup, which is when they run elsewhere. */
weak void __wasm_call_ctors(void);
void _init(void)
{
	if (__wasm_call_ctors) __wasm_call_ctors();
}
int __libc_start_main(int (*)(), int, char **,
	void (*)(), void(*)(), void(*)());

static int start(void *sp)
{
	long *p = sp;
	int argc = p[0];
	char **argv = (void *)(p+1);
	return __libc_start_main((int (*)())main_shim, argc, argv, _init, _fini, 0);
}

int __gex_fork_run(int (*)(void *), void *);

hidden void __gex_start_c(long *p)
{
	/* under fork's driver (src/process/wasm32/__gex_fork.c) */
	__gex_fork_run(start, p);
}

/* Both entries in the module's table, where gex finds them by name. */
int __gex_start(int, int, int, long long);
int __gex_thread_start(int, int, int, long long);
__attribute__((used)) void *const __gex_entries[] = { __gex_start, __gex_thread_start };
