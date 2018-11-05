#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  uint gid;
  char* path;

  path = argv[2];
  gid = atoi(argv[1]);
  if (chgrp(path, gid) == -1) {
    printf(1, "chgrp failed\n");
  }
  exit();
}

#endif
