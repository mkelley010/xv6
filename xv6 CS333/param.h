#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
// #define FSSIZE       1000  // size of file system in blocks
#define FSSIZE       2000  // size of file system in blocks  // CS333 requires a larger FS.
#ifdef CS333_P2
#define DEFAULTUID   0   // default uid of first process in P2 & beyond and files created by mkfs in P5
#define DEFAULTGID   0   // default gid of first process in P2 & beyond and files created by mkfs in P5
#endif
#ifdef CS333_P3P4
#define MAXPRIO      6   // max num of priority lists
#define TICKS_TO_PROMOTE 3000 // promotion timer
#define BUDGET       300 // proc budget
#endif
#ifdef CS333_P5
#define DEFAULTMODE  00755 // default mode for files
#endif

