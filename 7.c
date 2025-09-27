#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_LEN 1024
#define MAX_ARGS 100 //This one accepts arguments

int main() {
    char input[MAX_INPUT_LEN];

    while (1) {
        //Same prompt
        printf("MoreShell> ");
        fflush(stdout);

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        input[strcspn(input, "\n")] = '\0';
        
        //Exit shell if user types exit
        if (strcmp(input, "exit") == 0) {
            printf("Exiting MoreShell.\n");
            break;
        }

        //parse input into arguments
        char *args[MAX_ARGS];
        int argc = 0;
        //seperates the input into multiple arguments
        char *token = strtok(input, " ");
        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        //Fork and exec
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            //Child process
            execvp(args[0], args);
            perror("exec failed");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
        }
    }

    return 0;
}
