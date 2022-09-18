#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  char *exec_argvs[MAXARG];
  
  int argvs_index = 0;
  for (int i = 1; i < argc; i++){
    exec_argvs[argvs_index++] = argv[i];
  }
  
  char buf[1];  //optimazation:increase buffer size.
  int temp = argvs_index;
  int line_index = 0;
  char line[512];
  while(read(0, buf, 1) != 0){
    //the end of input line, append it and exec.
    if (buf[0] == '\n'){
      line[line_index] = 0;
      line_index = 0;

      exec_argvs[temp++] = line;
      exec_argvs[temp] = 0;
      temp = argvs_index; //ready for reading next std input line.
      int pid = fork();
      if (pid == 0){
        exec(exec_argvs[0], exec_argvs);
        printf("echo error\n");
      } else if (pid > 0){
        wait(0);
      } else {
        printf("fork error");
        exit(1);
      }
    } else if (buf[0] == ' '){
      line[line_index] = 0;
      line_index = 0;
      exec_argvs[temp++] = line;
    } else {
      line[line_index++] = buf[0];
    }
  }
  exit(0);
}