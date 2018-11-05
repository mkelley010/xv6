#ifdef CS333_P2
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  int end = 0, total = 0;
  int now = uptime();
  int pid = fork();
  if (pid < 0) {
    printf(1, "Fork failed!\n");
    exit();
  }
  else if (pid == 0) {
    exec(argv[1], argv + 1);
    exit();
  }
  else {
    wait();
    end = uptime();
    total = end - now;
  }
  if (total < 10) {
    printf(1, "%s ran in %d.00%d seconds\n", argv[1], total / 1000, total % 1000);
  }
  else if (total > 10 && total < 100)
  {
    printf(1, "%s ran in %d.0%d seconds\n", argv[1], total / 1000, total % 1000);
  }
  else {
    printf(1, "%s ran in %d.%d seconds\n", argv[1], total / 1000, total % 1000);
  }
  exit();
}

#endif
