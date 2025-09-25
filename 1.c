#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// error helper
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

//  this writes N copies of a character to file descriptor
static void write_repeated(int fd, char ch, long count) {
    for (long i = 0; i < count; i++) {
        if (write(fd, &ch, 1) != 1) die("write");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_bits_file> <dest_compressed_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0) die("open source");
    int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) die("open dest");

    char c, prev = 0;
    long run = 0;
    int have_prev = 0;

    while (1) {
        ssize_t r = read(in_fd, &c, 1);
        if (r < 0) die("read");
        if (r == 0) break; // EOF

        if (c == '0' || c == '1') {
            if (!have_prev) {
                prev = c;
                run = 1;
                have_prev = 1;
            } else if (c == prev) {
                run++;
            } else {
                // flush previous run
                if (run >= 16) {
                    char buf[64];
                    if (prev == '1') snprintf(buf, sizeof(buf), "+%ld+", run);
                    else snprintf(buf, sizeof(buf), "-%ld-", run);
                    if (write(out_fd, buf, strlen(buf)) < 0) die("write");
                } else {
                    write_repeated(out_fd, prev, run);
                }
                prev = c;
                run = 1;
            }
        } else if (c == ' ' || c == '\n') {
            // flush any pending run
            if (have_prev) {
                if (run >= 16) {
                    char buf[64];
                    if (prev == '1') snprintf(buf, sizeof(buf), "+%ld+", run);
                    else snprintf(buf, sizeof(buf), "-%ld-", run);
                    if (write(out_fd, buf, strlen(buf)) < 0) die("write");
                } else {
                    write_repeated(out_fd, prev, run);
                }
                have_prev = 0;
                run = 0;
            }
            // this  writes the  separator
            if (write(out_fd, &c, 1) != 1) die("write");
        }
    }

    // this flushes the leftover run
    if (have_prev) {
        if (run >= 16) {
            char buf[64];
            if (prev == '1') snprintf(buf, sizeof(buf), "+%ld+", run);
            else snprintf(buf, sizeof(buf), "-%ld-", run);
            if (write(out_fd, buf, strlen(buf)) < 0) die("write");
        } else {
            write_repeated(out_fd, prev, run);
        }
    }

    close(in_fd);
    close(out_fd);
    return EXIT_SUCCESS;
}

