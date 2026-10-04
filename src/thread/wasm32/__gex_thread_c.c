#include "pthread_impl.h"

void __wasm_init_tls(void *);
void __gex_set_tp(uintptr_t);

/* A clone's child (see wasm32 __clone and gex_thread.s). */
_Noreturn hidden void __gex_thread_c(unsigned long *sp)
{
	int (*func)(void *) = (int (*)(void *))sp[0];
	void *arg = (void *)sp[1];
	pthread_t td = (pthread_t)sp[2];
	if (td) {
		__gex_set_tp((uintptr_t)td);
		__wasm_init_tls(td->dtv);
	}
	int r = func(arg);
	for (;;) __syscall(SYS_exit, r);
}
