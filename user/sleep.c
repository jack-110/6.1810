#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc == 1) {
    printf("The command argument is null.");
  }
  
  int sleep_time = atoi(argv[1]);
  
  int res = sleep(sleep_time);
  if (res != 0) {
    printf("There some errors in kernel.");
  }
  
  exit(0);
}