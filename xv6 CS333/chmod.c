#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  char* path;
  uint mode;

  // error check for invalid inputs
  if (strlen(argv[1]) != 4) {
    printf(1, "chmod failed - mode must be 4 digits exactly\n");
    exit();
  }
  if (argv[1][0] != '0' && argv[1][0] != '1') {
    printf(1, "chmod failed - setuid must be 0 or 1\n");
    exit();
  }
  for (int i = 1; i < 4; i++) {
    if (argv[1][i] == '8' || argv[1][i] == '9') {
      printf(1, "chmod failed - permissions must be 0-7\n");
      exit();
    }
  }

  mode = atoo(argv[1]);
  path = argv[2];
  if ((chmod(path, mode) == -1)) {
    printf(1, "chmod failed\n");
  }
  exit();
}

#endif
