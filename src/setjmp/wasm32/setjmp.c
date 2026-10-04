/* wasm32: setjmp/longjmp are lowered by clang (-mllvm -wasm-enable-sjlj,
 * part of gex's wasm32 flags) onto wasm exception handling; this is the
 * runtime that lowering calls (wasi-libc's layout) and the tag it throws.
 * A setjmp call that was not lowered (code compiled without the flag)
 * cannot work and returns 0; longjmp always throws. */
#include <setjmp.h>
#include <stdint.h>

__asm__(
	"\t.tagtype __c_longjmp i32\n"
	"\t.globl __c_longjmp\n"
	"__c_longjmp:\n"
);

struct jmp_buf_impl
{
	void *func_invocation_id;
	uint32_t label;
	struct { void *env; int val; } arg;
};

void __wasm_setjmp(void *env, uint32_t label, void *func_invocation_id)
{
	struct jmp_buf_impl *jb = env;
	jb->func_invocation_id = func_invocation_id;
	jb->label = label;
}

uint32_t __wasm_setjmp_test(void *env, void *func_invocation_id)
{
	struct jmp_buf_impl *jb = env;
	return jb->func_invocation_id == func_invocation_id ? jb->label : 0;
}

_Noreturn void __wasm_longjmp(void *env, int val)
{
	struct jmp_buf_impl *jb = env;
	jb->arg.env = env;
	jb->arg.val = val ? val : 1;
	__builtin_wasm_throw(1, &jb->arg);
}

int setjmp(jmp_buf env) { return 0; }
int _setjmp(jmp_buf env) { return 0; }
_Noreturn void longjmp(jmp_buf env, int val) { __wasm_longjmp(env, val); }
_Noreturn void _longjmp(jmp_buf env, int val) { __wasm_longjmp(env, val); }
