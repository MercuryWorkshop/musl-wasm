/* wasm32 (gex): shared libraries' constructors and thread-local blocks
 * (gex_dso.h). */
#include "gex_dso.h"
#include "libc.h"

/* version 1; the loader checks it and max before writing */
struct __gex_dsos __gex_dsos = { 1, 0, GEX_DSO_MAX };

/* Every library's constructors, before the program's (crt1's _init). */
void __gex_dso_init(void)
{
	for (unsigned i = 0; i < __gex_dsos.count; i++)
		if (__gex_dsos.dso[i].ctors) __gex_dsos.dso[i].ctors();
}

/* Point every library's __tls_base at its block of the thread's TLS area
 * at dtv, copying its initial image when init. */
hidden void __gex_dso_tls(unsigned char *dtv, int init)
{
	for (unsigned i = 0; i < __gex_dsos.count; i++) {
		struct __gex_dso *d = &__gex_dsos.dso[i];
		if (!d->tls_size) continue;
		void *b = dtv + d->tls_offset;
		if (init) d->tls_init(b);
		else d->tls_set(b);
	}
}
