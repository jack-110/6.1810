#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p[2];
  int q[2];
  pipe(p);
  pipe(q);

  int pid = fork();
  if (pid > 0) {
    //send the message ping to child
    close(p[0]);
    write(p[1], "ping", 4);
    close(p[1]);

    //read the message pong from child    
    close(q[1]);
    char buffer[4];
    read(q[0], buffer, 4);
    close(q[0]);

    int pid = getpid();
    printf("%d: received %s\n", pid, buffer);
  } else if (pid == 0) {
    //read the message ping from parent
    close(p[1]);
    char buffer[4];
    read(p[0], buffer, 4);
    close(p[0]);

    int pid = getpid();
    printf("%d: received %s\n", pid, buffer);

    //send message pong to parent
    close(q[0]);
    write(q[1], "pong", 4);
    close(q[1]);
  } else {
    printf("fork error");
    exit(1);
  }
  
  exit(0);
}