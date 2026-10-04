/* The i386 kernel's struct stat64, which gex-kernel's i386 ABI writes.
 * i386 aligns 64-bit members to 4 bytes; wasm32 aligns them to 8, so the
 * 64-bit members are declared with 4-byte alignment to keep the kernel's
 * offsets (st_size at 44, st_ino at 88, 96 bytes in all). */
typedef long long __attribute__((__aligned__(4))) __kll;
typedef unsigned long long __attribute__((__aligned__(4))) __kull;
struct kstat {
	__kull st_dev;
	int __st_dev_padding;
	long __st_ino_truncated;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	__kull st_rdev;
	int __st_rdev_padding;
	__kll st_size;
	blksize_t st_blksize;
	__kll st_blocks;
	long st_atime_sec;
	long st_atime_nsec;
	long st_mtime_sec;
	long st_mtime_nsec;
	long st_ctime_sec;
	long st_ctime_nsec;
	__kull st_ino;
} __attribute__((__aligned__(4)));
_Static_assert(sizeof(struct kstat) == 96, "i386 stat64 is 96 bytes");
