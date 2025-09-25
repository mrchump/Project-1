#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// write N copies of a character
static void write_repeated(int fd, char ch, long count) {
    for (long i = 0; i < count; i++) {
        if (write(fd, &ch, 1) != 1) die("write");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_compressed_file> <dest_bits_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0) die("open source");
    int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) die("open dest");

    char c;
    while (read(in_fd, &c, 1) == 1) {
        if (c == '+' || c == '-') {
            int is_one = (c == '+');
            char numbuf[64];
            size_t n = 0;

            // read number
            while (1) {
                char d;
                if (read(in_fd, &d, 1) != 1) die("read number");
                if (isdigit((unsigned char)d)) {
                    if (n >= sizeof(numbuf)-1) die("number too long");
                    numbuf[n++] = d;
                } else {
                    // expect closing marker
                    if ((is_one && d != '+') || (!is_one && d != '-')) {
                        fprintf(stderr, "Malformed compressed sequence\n");
                        exit(EXIT_FAILURE);
                    }
                    numbuf[n] = '\0';
                    long count = strtol(numbuf, NULL, 10);
                    write_repeated(out_fd, is_one ? '1' : '0', count);
                    break;
                }
            }
        } else {
            // copy char directly (0,1, space, newline)
            if (write(out_fd, &c, 1) != 1) die("write");
        }
    }

    close(in_fd);
    close(out_fd);
    return EXIT_SUCCESS;
}

