#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  uint uid;
  char* path;

  path = argv[2];
  uid = atoi(argv[1]);
  if (chown(path, uid) == -1) {
    printf(1, "chown failed\n");
  }
  exit();
}

#endif
