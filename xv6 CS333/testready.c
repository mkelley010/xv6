#ifdef CS333_P3P4
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  int n = 10;
  int pid = 0;
  int j = 0;
  for (int i = 0; i < n; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      while (j < 1000000000) {
        j++;
      }
    }
  }
  exit();
}
#endif
