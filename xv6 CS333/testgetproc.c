#ifdef CS333_P2
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  int n = 50;
  int pid = 0;
  for (int i = 0; i < n; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      exec("sh", argv + 1);
    }
  }
  exit();
}
#endif
