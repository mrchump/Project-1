#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_bits_file> <dest_compressed_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // Child process: execute MyCompress
        execl("./MyCompress", "MyCompress", argv[1], argv[2], (char*)NULL);
        perror("execl");  // only runs if execl fails
        _exit(127);
    } else {
        // Parent: wait for child
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Compression finished successfully.\n");
        } else {
            fprintf(stderr, "Compression failed (status=%d).\n", status);
        }
    }

    return 0;
}

