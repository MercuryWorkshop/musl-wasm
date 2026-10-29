/* wasm32 (gex): the shared libraries gex's loader placed, for libc to give
 * each its thread-local block and run its constructors (docs/gexcc.md 7.4).
 * The loader fills __gex_dsos in libc's data before the program starts,
 * in initialisation order (each library after those it needs); function
 * pointers are table indices of each module's own functions. A static
 * program has none. */
#ifndef GEX_DSO_H
#define GEX_DSO_H
#include <stddef.h>
#include <features.h>

struct __gex_dso {
	size_t tls_size, tls_align;
	/* the module's __wasm_init_tls (copy .tdata into the block, point
	 * __tls_base at it) and __gex_set_tls_base (point it only) */
	void (*tls_init)(void *);
	void (*tls_set)(void *);
	void (*ctors)(void);
	/* the block's offset in a thread's TLS area (libc's own block first),
	 * laid out by __init_tls */
	size_t tls_offset;
};

#define GEX_DSO_MAX 1024
struct __gex_dsos {
	unsigned version, count, max;
	struct __gex_dso dso[GEX_DSO_MAX];
};

extern struct __gex_dsos __gex_dsos;
hidden void __gex_dso_tls(unsigned char *, int);
#endif
