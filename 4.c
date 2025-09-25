#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_bits_file> <dest_compressed_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fds[2];
    if (pipe(fds) < 0) die("pipe");

    // ---- Reader child ----
    pid_t rpid = fork();
    if (rpid < 0) die("fork reader");

    if (rpid == 0) {
        close(fds[0]);  // close read end

        int src = open(argv[1], O_RDONLY);
        if (src < 0) die("open source");

        char buf[4096];
        ssize_t n;
        while ((n = read(src, buf, sizeof(buf))) > 0) {
            if (write(fds[1], buf, n) != n) die("write pipe");
        }
        if (n < 0) die("read src");

        close(src);
        close(fds[1]);
        _exit(0);
    }

    // ---- Compressor child ----
    pid_t cpid = fork();
    if (cpid < 0) die("fork compressor");

    if (cpid == 0) {
        close(fds[1]);  // close unused write end

        // connect pipe → stdin
        if (dup2(fds[0], STDIN_FILENO) < 0) die("dup2 stdin");
        close(fds[0]);

        // open destination and connect → stdout
        int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dst < 0) die("open dest");
        if (dup2(dst, STDOUT_FILENO) < 0) die("dup2 stdout");
        close(dst);

        // run MyCompress (which reads stdin, writes stdout)
        execl("./MyCompress", "MyCompress", "/dev/stdin", "/dev/stdout", (char*)NULL);
        perror("execl MyCompress");
        _exit(127);
    }

    // ---- Parent ----
    close(fds[0]);
    close(fds[1]);

    int st;
    waitpid(rpid, &st, 0);
    waitpid(cpid, &st, 0);

    return 0;
}
