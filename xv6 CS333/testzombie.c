#ifdef CS333_P3P4
#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i, pid;
  for (i=0; i<5; i++) {
    pid = fork();
    if (pid == 0) {   // child)
      sleep(5 * TPS);
      exit();
    }
  }
  sleep(10 * TPS);  // sleep 10 seconds
  while (wait() != -1)
    wait();

  exit();
}
#endif

