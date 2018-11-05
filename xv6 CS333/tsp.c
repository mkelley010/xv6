// Credit to Jacob Collins for some of the test code and ideas

#ifdef CS333_P3P4
#include "types.h"
#include "user.h"
#include "param.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

void setPrio();
void maxPrio();
void printMenu();
void setPrioInvalid();
void getPrio();

int
main(int argc, char *argv[])
{
  int select, done;
  char buf[5];

  while(1) {
    done = FALSE;
    printMenu();
    printf(1, "Enter test number: ");
    gets(buf, 5);

    if ((buf[0] == '\n') || buf[0] == '\0') {
      continue;
    }

    select = atoi(buf);

    switch(select) {
      case 0:
        done = TRUE;
        break;
      case 1:
        setPrio();
        break;
      case 2:
        maxPrio();
        break;
      case 3:
        setPrioInvalid();
        break;
      case 4:
        getPrio();
        break;
      default:
        printf(1, "Error: invalid test number.\n");
    }

    if (done) {
      break;
    }
  }
  exit();
}

// Sets priority of children - used to show set priority places on correct list
void
setPrio()
{
  printf(1, "\n\n");
  int n = 10, pid = 0, j = 0, oldPrio = 6, newPrio = 2;
  for (int i = 0; i < n; i++)
  {
    pid = fork();
    if (pid == 0)
    {
      // change setpriority values to test moving
      setpriority(getpid(), oldPrio);
      while (j < 100000000) {
        j++;
      }
      printf(1, "Changing priority of process %d to %d\n", getpid(), newPrio);
      // change setpriority values to test moving
      setpriority(getpid(), newPrio);
      while (j < 1000000000) {
        j++;
      }
    }
  }
  exit();
  printf(1, "\n\n");
}

void
maxPrio()
{
  int prio = 5;
  printf(1, "\n\n");
  printf(1, "==TESTING SET PRIORITY WITH MAXPRIO SET TO %d==\n", MAXPRIO);
  printf(1, "Current proc priority is: %d\n", getpriority(getpid()));
  printf(1, "Setting priority to: %d\n", prio);
  int rc = setpriority(getpid(), prio);
  if (rc == -1) {
    printf(1, "Set priority failed - value out of bounds\n");
  }
  else if (rc == 0) {
    printf(1, "Priority set!\n");
  }
  printf(1, "Priority after attempting set: %d\n", getpriority(getpid()));
  printf(1, "Setting priority to: %d\n", 0);
  rc = setpriority(getpid(), 0);
  if (rc == -1) {
    printf(1, "Set priority failed - value out of bounds\n");
  }
  else if (rc == 0) {
    printf(1, "Priority set!\n");
  }
  printf(1, "Priority after attempting set: %d\n", getpriority(getpid()));
  printf(1, "\n\n");

}

void
printMenu()
{
  printf(1, "0. Exit\n");
  printf(1, "1. Set priority ordering test\n");
  printf(1, "2. Max prio test\n");
  printf(1, "3. Set invalid priority test\n");
  printf(1, "4. Get priority test\n");
}

void
setPrioInvalid()
{
  printf(1, "Current PID: %d\n", getpid());
  printf(1, "Current priority: %d\n", getpriority(getpid()));
  printf(1, "Changing priority to %d\n", MAXPRIO + 10);
  int rc = setpriority(getpid(), MAXPRIO + 10);
  if (rc == -1) {
    printf(1, "Failed to set - priority invalid or pid not found\n");
  }
  printf(1, "Priority is now: %d\n", getpriority(getpid()));
  printf(1, "Changing priority of process %d to %d\n", 100, 0);
  rc = setpriority(100, 0);
  if (rc == -1) {
    printf(1, "Failed to set - priority invalid or pid not found\n");
  }
  printf(1, "Priority is now: %d\n", getpriority(getpid()));

}

void
getPrio()
{
  printf(1, "Current PID is %d and its priority is %d\n", getpid(), getpriority(getpid()));
  printf(1, "Sleeping, ctrl-p now.\n");
  sleep(5000);
  printf(1, "init pid is %d and its priority is %d\n", 1, getpriority(1));
  printf(1, "Sleeping, ctrl-p now.\n");
  sleep(5000);
  printf(1, "Calling getpriority on pid %d\n", 10);
  int rc = getpriority(10);
  if (rc == -1) {
    printf(1, "pid not found\n");
  }
  else if (rc == 0) {
    printf(1, "success!\n");
  }

}
#endif
