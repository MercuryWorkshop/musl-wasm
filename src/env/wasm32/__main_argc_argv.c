/* wasm32 (gex): a program whose main takes no arguments defines
 * __original_main (clang's name for main(void)); crt1 calls
 * __main_argc_argv, which then comes from here. A program with
 * main(int, char **) defines __main_argc_argv itself, and this member is
 * not linked. */
#include "libc.h"

int __original_main(void);

weak int __main_argc_argv(int argc, char **argv)
{
	return __original_main();
}
