#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 100
#define MAX_CMDS 10

void parse_command(char *cmd, char **args){
  int argc = 0;
  char *token = strtok(cmd, " ");
  while (token != NULL && argc < MAX_ARGS - 1){
    args[argc++] = token;
    token = strtok(NULL, " ");
  }
  args[argc++] = NULL;
}
int main(){
  char input[MAX_INPUT];
  char *cmds[MAX_CMDS];

  while(1){
    printf("DupShell>>");
    fflush(stdout);

  if (fgets(input, sizeof(input), stdin) == NULL) {
    printf("\n");
    break;
  }

  size_t len = strlen(input);
  if (len > 0 && input[len - 1 ] == '\n'){
    input[len - 1] = '\0';
  }

  if (strcmp(input, "exit") == 0 ){
    break;
  }

  int ncmds = 0;
  char *token = strtok(input, "|");
  while(token != NULL && ncmds < MAX_CMDS){
    cmds[ncmds++] = token;
    token = strtok(NULL, "|");
  }
  int prev_fd = -1;
  for(int i = 0; i < ncmds; i++){
    char *args[MAX_ARGS];
    parse_command(cmds[i], args);

    int fd[2];
    if(i < ncmds - 1){
      if(pipe(fd) < 0 ){
        perror("pipe has failed");
        exit(1);
      }
    }

    pid_t pid = fork();
    if (pid == 0){
      if ( i > 0 ){
        dup2(prev_fd , STDIN_FILENO);
        close(prev_fd);
      }
      if (i < ncmds - 1){
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
      }
      execvp(args[0], args);
      perror("exec failed");
      exit(1);
    }
    else if(pid > 0){
      if (i > 0 )close(prev_fd);
      if(i > ncmds - 1){
        close(fd[1]);
        prev_fd = fd[0];
      }
    }
    else {
      perror("failed");
      exit(1);
    }
  }
  for(int i = 0; i < ncmds; i++){
    wait(NULL);
  }

}
return 0;
}
