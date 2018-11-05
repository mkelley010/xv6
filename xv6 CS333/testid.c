#ifdef CS333_P2
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  uint uid, gid, ppid;
  uid = getuid();
  printf(2, "UID TESTS\n");
  printf(2, "---------\n");
  printf(2, "Current UID: %d\n", uid);
  // check set uid in bounds
  printf(2, "Setting UID to 100\n");
  if (setuid(100) == 0)
  {
    uid = getuid();
    printf(2, "UID is now: %d\n", uid);
  }
  else
  {
     printf(2, "Test failed!\n");
  }
  // check set uid below 0
  printf(2, "Setting UID to -25\n");
  if (setuid(-25) == -1)
  {
    uid = getuid();
    printf(2, "Set UID failed. Value was out of bounds. UID is still: %d\n", uid);
  }
  else
  {
     printf(2, "Test failed!\n");
  }
  // check set uid above 32767
  printf(2, "Setting UID to 99999\n");
  if (setuid(99999) == -1)
  {
    uid = getuid();
    printf(2, "Set UID failed. Value was out of bounds. UID is still: %d\n", uid);
  }
  else
  {
     printf(2, "Test failed!\n");
  }

  printf(2, "\n");
  printf(2, "GID TESTS\n");
  printf(2, "---------\n");
  gid = getgid();
  printf(2, "Current GID: %d\n", gid);
  // checck gid in bounds
  printf(2, "Setting GID to 100\n");
  if (setgid(100) == 0)
  {
    gid = getgid();
    printf(2, "GID is now: %d\n", gid);
  }
  else
  {
     printf(2, "Test failed!\n");
  }
  // check gid below 0
  printf(2, "Setting GID to -25\n");
  if (setgid(-25) == -1)
  {
    gid = getgid();
    printf(2, "Set GID failed. Value was out of bonds. GID is still: %d\n", gid);
  }
  else
  {
     printf(2, "Test failed!\n");
  }
  // check gid above 32767
  printf(2, "Setting GID to 99999\n");
  if (setgid(99999) == -1)
  {
    gid = getgid();
    printf(2, "Set GID failed. Value was out of bonds. GID is still: %d\n", gid);
  }
  else
  {
     printf(2, "Test failed!\n");
  }

  printf(2, "\n");
  printf(2, "PPID TEST\n");
  printf(2, "----------\n");
  ppid = getppid();
  printf(2, "PID is: %d\n", getpid());
  printf(2, "PPID is: %d\n", ppid);

  exit();
}
#endif
