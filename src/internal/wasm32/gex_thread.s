# wasm32 (gex): per-thread state as module globals. gex instantiates the
# program's module once per thread, so a module global is a thread's own.
#
#   __gex_ctx   the thread's gex register context (syscalls go through it)
#   __gex_tp    the thread pointer (struct pthread *)
#
# and the two entries gex binds in its function table, both with gex's
# F_body signature (ctx, depth, need, ret) -> i32, neither returning:
#
#   __gex_start          the image's entry (AT_ENTRY): ESP points at argc
#   __gex_thread_start   AT_ENTRY + 16: a clone's child, ESP points at
#                        (func, arg, tls) left by __clone

	.globaltype __stack_pointer, i32

	.globaltype __gex_ctx, i32
__gex_ctx:
	.globaltype __gex_tp, i32
__gex_tp:

	.globl __gex_get_ctx
	.type __gex_get_ctx,@function
__gex_get_ctx:
	.functype __gex_get_ctx () -> (i32)
	global.get __gex_ctx
	end_function

	.globl __gex_get_tp
	.type __gex_get_tp,@function
__gex_get_tp:
	.functype __gex_get_tp () -> (i32)
	global.get __gex_tp
	end_function

	.globl __gex_set_tp
	.type __gex_set_tp,@function
__gex_set_tp:
	.functype __gex_set_tp (i32) -> ()
	local.get 0
	global.set __gex_tp
	end_function

	.functype __gex_start_c (i32) -> ()
	.functype __gex_thread_c (i32) -> ()

	.globl __gex_start
	.type __gex_start,@function
__gex_start:
	.functype __gex_start (i32, i32, i32, i64) -> (i32)
	.local i32
	local.get 0
	global.set __gex_ctx
	# esp: the low word of ctx slot x(4)
	local.get 0
	i32.load 32
	local.tee 4
	# the C stack starts below argc, 16-aligned
	i32.const 64
	i32.sub
	i32.const -16
	i32.and
	global.set __stack_pointer
	local.get 4
	call __gex_start_c
	unreachable
	end_function

	.globl __gex_thread_start
	.type __gex_thread_start,@function
__gex_thread_start:
	.functype __gex_thread_start (i32, i32, i32, i64) -> (i32)
	.local i32
	local.get 0
	global.set __gex_ctx
	local.get 0
	i32.load 32
	local.tee 4
	i32.const -16
	i32.and
	i32.const 16
	i32.sub
	global.set __stack_pointer
	local.get 4
	call __gex_thread_c
	unreachable
	end_function
