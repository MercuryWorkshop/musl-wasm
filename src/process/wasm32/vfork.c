/* wasm32 (gex): vfork is fork (src/process/wasm32/__gex_fork.c); a vfork
 * child that only execs or _exits cannot tell the difference. */
#define _GNU_SOURCE
#include <unistd.h>

pid_t vfork(void)
{
	return fork();
}
