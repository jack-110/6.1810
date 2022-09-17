#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
filter (int *feeds, int size) 
{ 
  if (size <= 0) {
    return;
  }


  int p[2];
  pipe(p);

  int pid = fork();
  if (pid > 0) {
    close(p[0]);
    int buffer[1];
    for (int i = 0; i < size; i++) {
      if (i == 0) {
        printf("prime %d\n", feeds[0]);
      } else {
        if (feeds[i] % feeds[0] != 0) {
          buffer[0] = feeds[i];
          write(p[1], buffer, sizeof(buffer));
        }
      }      
    }
    close(p[1]);
    wait(0);
  } else if (pid == 0) {
    close(p[1]);
    int feeds[1];
    int prime = 0;
    int j = 0;
    int buffer[size * sizeof(int)];
    while(read(p[0], feeds, sizeof(feeds)) != 0) {
      if (j == 0) {
        prime = feeds[0];
        printf("prime %d\n", feeds[0]);
        j++;
      } else {
          if (feeds[0] % prime != 0) {
            buffer[j-1] = feeds[0];
            j++;
           }      
      }
    }
    close(p[0]);

    filter(buffer, j - 1);
  } else {
    printf("fork error");
    exit(1);
  }
  exit(0);
}


int
main(int argc, char *argv[])
{

  int feeds[34] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 
                    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};
  filter(feeds, sizeof(feeds) / sizeof(int));
  
  exit(0);
}