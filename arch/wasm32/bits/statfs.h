/* The i386 kernel's struct statfs64 (84 bytes; 64-bit members 4-aligned). */
struct statfs {
	unsigned long f_type, f_bsize;
	fsblkcnt_t f_blocks, f_bfree, f_bavail;
	fsfilcnt_t f_files, f_ffree;
	fsid_t f_fsid;
	unsigned long f_namelen, f_frsize, f_flags, f_spare[4];
} __attribute__((__packed__, __aligned__(4)));
