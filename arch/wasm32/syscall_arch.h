/* wasm32 under gex: a syscall is the i386 Linux ABI, issued through gex's
 * `gex.syscall(ctx, pc)` import. The number and the arguments go into the
 * calling thread's register context (ctx slots are 8 bytes; gex-kernel's
 * i386 ABI reads eax, ebx, ecx, edx, esi, edi, ebp) and the result comes
 * back in eax. The context is the one gex entered the thread with
 * (__gex_get_ctx, set by the thread's entry). */

#define __SYSCALL_LL_E(x) \
((union { long long ll; long l[2]; }){ .ll = x }).l[0], \
((union { long long ll; long l[2]; }){ .ll = x }).l[1]
#define __SYSCALL_LL_O(x) __SYSCALL_LL_E((x))

unsigned long __gex_get_ctx(void);
__attribute__((__import_module__("gex"), __import_name__("syscall")))
void __gex_syscall_import(int, long long);

/* ctx slots: x(n) = 8 * n. eax 0, ecx 1, edx 2, ebx 3, esp 4, ebp 5,
 * esi 6, edi 7; PC is slot 32. */
static inline long __gex_sc(long n, long a, long b, long c, long d, long e, long f)
{
	volatile unsigned long long *x = (volatile unsigned long long *)__gex_get_ctx();
	x[0] = (unsigned long)n;
	x[3] = (unsigned long)a;
	x[1] = (unsigned long)b;
	x[2] = (unsigned long)c;
	x[6] = (unsigned long)d;
	x[7] = (unsigned long)e;
	x[5] = (unsigned long)f;
	__gex_syscall_import((int)(unsigned long)x, 0);
	return (long)(unsigned long)x[0];
}

static inline long __syscall0(long n)
{ return __gex_sc(n, 0, 0, 0, 0, 0, 0); }
static inline long __syscall1(long n, long a1)
{ return __gex_sc(n, a1, 0, 0, 0, 0, 0); }
static inline long __syscall2(long n, long a1, long a2)
{ return __gex_sc(n, a1, a2, 0, 0, 0, 0); }
static inline long __syscall3(long n, long a1, long a2, long a3)
{ return __gex_sc(n, a1, a2, a3, 0, 0, 0); }
static inline long __syscall4(long n, long a1, long a2, long a3, long a4)
{ return __gex_sc(n, a1, a2, a3, a4, 0, 0); }
static inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{ return __gex_sc(n, a1, a2, a3, a4, a5, 0); }
static inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6)
{ return __gex_sc(n, a1, a2, a3, a4, a5, a6); }

