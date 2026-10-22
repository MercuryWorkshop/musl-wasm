typedef int32_t int_fast16_t;
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast16_t;
typedef uint32_t uint_fast32_t;

#define INT_FAST16_MIN  INT32_MIN
#define INT_FAST32_MIN  INT32_MIN

#define INT_FAST16_MAX  INT32_MAX
#define INT_FAST32_MAX  INT32_MAX

#define UINT_FAST16_MAX UINT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

/* intptr_t, ptrdiff_t and size_t are long here (_Addr), so their limits
 * are long too: an int-typed SIZE_MAX makes a type-generic comparison of a
 * size_t with it pick neither branch. */
#define INTPTR_MIN      (-1-0x7fffffffL)
#define INTPTR_MAX      0x7fffffffL
#define UINTPTR_MAX     0xffffffffUL
#define PTRDIFF_MIN     (-1-0x7fffffffL)
#define PTRDIFF_MAX     0x7fffffffL
#define SIZE_MAX        0xffffffffUL
