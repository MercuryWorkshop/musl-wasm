#include <stdlib.h>
#include "syscall.h"

int __gexw_vfork_active(void);
_Noreturn void __gexw_vfork_exit(int);

_Noreturn void _Exit(int ec)
{
	/* in a vfork child (wasm32, __gexw_vfork.c): the parent goes on */
	if (__gexw_vfork_active()) __gexw_vfork_exit(ec);
	__syscall(SYS_exit_group, ec);
	for (;;) __syscall(SYS_exit, ec);
}
