#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"

int
main(void)
{
  uint max = 64;
  int numProcs = 0, elapsedAfterDec = 0, cpuAfterDec = 0;
  struct uproc *ptable = malloc(max*sizeof(struct uproc));
  numProcs = getprocs(max, ptable);
  if (numProcs == -1) {
    printf(1, "ps failed.\n");
    exit();
  }
#ifdef CS333_P3P4
  printf(1, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t", "PID", "Name", "UID", "GID", "PPID", "Prio", "Elapsed", "CPU", "State", "Size");
  printf(1, "\n");
  for (int i = 0; i < numProcs; i++)  {
    // format for decimal output
    elapsedAfterDec = ptable[i].elapsed_ticks % 1000;
    cpuAfterDec = ptable[i].CPU_total_ticks % 1000;
    printf(1, "%d\t%s\t%d\t%d\t%d\t%d\t%d.%d\t%d.%d\t%s\t%d\t\n", ptable[i].pid, ptable[i].name, ptable[i].uid, ptable[i].gid, ptable[i].ppid, ptable[i].priority, ptable[i].elapsed_ticks / 1000, elapsedAfterDec, ptable[i].CPU_total_ticks / 100, cpuAfterDec, ptable[i].state, ptable[i].size);
  }
#else
  printf(1, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t", "PID", "Name", "UID", "GID", "PPID", "Elapsed", "CPU", "State", "Size");
  printf(1, "\n");
  for (int i = 0; i < numProcs; i++)  {
    // format for decimal output
    elapsedAfterDec = ptable[i].elapsed_ticks % 1000;
    cpuAfterDec = ptable[i].CPU_total_ticks % 1000;
    printf(1, "%d\t%s\t%d\t%d\t%d\t%d.%d\t%d.%d\t%s\t%d\t\n", ptable[i].pid, ptable[i].name, ptable[i].uid, ptable[i].gid, ptable[i].ppid, ptable[i].elapsed_ticks / 1000, elapsedAfterDec, ptable[i].CPU_total_ticks / 100, cpuAfterDec, ptable[i].state, ptable[i].size);
  }
#endif
  free(ptable);
  exit();
}
#endif
