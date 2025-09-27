#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 100

int main() {
    char command[MAX_CMD_LEN];

    while (1) {
        //shell> is the display prompt
        printf("shell> ");
        fflush(stdout);

        //Read user input 
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        //Exit shell if user types exi t
        if (strcmp(command, "exit") == 0) {
            printf("Exiting MiniShell.\n");
            break;
        }

        //Fork child process
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            //Child process
            char *args[] = {command, NULL};//Turns command into single arrray to then be used
            execvp(command, args);//execvp replaces the clones child process with the command

            //error
            perror("exec failed");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
        }
    }

    return 0;
}
