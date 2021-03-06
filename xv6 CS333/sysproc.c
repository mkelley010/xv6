#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "uproc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  xticks = ticks;
  return xticks;
}

//Turn off the computer
int
sys_halt(void)
{
  cprintf("Shutting down ...\n");
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}

//Print date in utc
#ifdef CS333_P1
int
sys_date(void)
{
  struct rtcdate *d;
  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
    return -1;
  else{
    cmostime(d);
    return 0;
    }
}
#endif

#ifdef CS333_P2
uint
sys_getuid(void)
{
  return proc->uid;
}

uint
sys_getgid(void)
{
  return proc->gid;
}

uint
sys_getppid(void)
{
  if (!proc->parent)
  {
    return proc->pid;
  }
  else
  {
    return (*proc).parent->pid;
  }
}

int
sys_setuid(void)
{
  int n;
  if (argint(0, &n) < 0) {
    return -1;
  }
  else if (n < 0 || n > 32767) {
    return -1;
  }
  else {
    proc->uid = n;
    return 0;
  }
}

int
sys_setgid(void)
{
  int n;
  if (argint(0, &n) < 0) {
    return -1;
  }
  else if (n < 0 || n > 32767) {
    return -1;
  }
  else {
    proc->gid = n;
    return 0;
  }
}

int
sys_getprocs(void)
{

  struct uproc *uprocs;
  int max;
  int numProcs = 0;
  if (argint(0, &max) < 0 ) {
    return -1;
  }
  if (argptr(1, (void*)&uprocs, sizeof(struct uproc)) < 0)
  {
    return -1;
  }
  if (max < 0 || max > 32767)
  {
    return -1;
  }
  max = (uint) max;
  numProcs = uprocHelper(max, uprocs);
  return numProcs;
}
#endif

#ifdef CS333_P3P4
int
sys_setpriority(void)
{
  int priority;
  int pid;
  if (argint(0, &pid) < 0) {
    return -1;
  }
  if (argint(1, &priority) < 0) {
    return -1;
  }
  if (priority < 0 || priority > MAXPRIO) {
    return -1;
  }
  int rc = setPriorityHelper(pid, priority);
  return rc;
}

int
sys_getpriority(void)
{
  int pid;
  int priority;
  if (argint(0, &pid) < 0) {
    return -1;
  }
  priority = getPriorityHelper(pid);
  return priority;
}
#endif

