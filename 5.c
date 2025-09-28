#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define OVERLAP 32

void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// writes a character multiple times
void write_repeated(int fd, char ch, long count) {
    for (long i = 0; i < count; i++) {
        if (write(fd, &ch, 1) != 1) die("write");
    }
}

// \;sCompresses a chunk of input file and writes to temp output
void compress_chunk(const char *filename, off_t start, off_t length, int chunk_id) {
    int in_fd = open(filename, O_RDONLY);
    if (in_fd < 0) die("open input");

    //find starting position of the chunk
    if (lseek(in_fd, start, SEEK_SET) == -1) die("lseek");

    //create a temp output file for this chunk
    char temp_name[32];
    snprintf(temp_name, sizeof(temp_name), "tmp_chunk_%d.txt", chunk_id);
    int out_fd = open(temp_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) die("open temp");

    char c, prev = 0;
    long run = 0;
    int have_prev = 0;
    off_t read_bytes = 0;

    // Read and compress the chunk
    while (read_bytes < length && read(in_fd, &c, 1) == 1) {
        read_bytes++;

        if (c == '0' || c == '1') {
            if (!have_prev) {
                prev = c;
                run = 1;
                have_prev = 1;
            } else if (c == prev) {
                run++;
            } else {
                if (run >= 16) {
                    dprintf(out_fd, "%c%ld%c", prev == '1' ? '+' : '-', run, prev == '1' ? '+' : '-');
                } else {
                    write_repeated(out_fd, prev, run);
                }
                prev = c;
                run = 1;
            }
        } else if (c == ' ' || c == '\n') {
            if (have_prev) {
                if (run >= 16) {
                    dprintf(out_fd, "%c%ld%c", prev == '1' ? '+' : '-', run, prev == '1' ? '+' : '-');
                } else {
                    write_repeated(out_fd, prev, run);
                }
                have_prev = 0;
            }
            write(out_fd, &c, 1);
        }
    }

    // Final flush
    if (have_prev) {
        if (run >= 16) {
            dprintf(out_fd, "%c%ld%c", prev == '1' ? '+' : '-', run, prev == '1' ? '+' : '-');
        } else {
            write_repeated(out_fd, prev, run);
        }
    }

    close(in_fd);
    close(out_fd);
}

//forks processes, compresses in parallel and merges output
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <output_file> <num_processes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int num_procs = atoi(argv[3]);
    if (num_procs <= 0) die("invalid number of processes");

    // Get size of input file
    int in_fd = open(input_file, O_RDONLY);
    if (in_fd < 0) die("open input");
    off_t file_size = lseek(in_fd, 0, SEEK_END);
    if (file_size == -1) die("lseek");
    close(in_fd);

    off_t chunk_size = file_size / num_procs;

    // Fork child processes to compress chunks
    for (int i = 0; i < num_procs; i++) {
        if (fork() == 0) {
            off_t start = i * chunk_size;
            off_t length = (i == num_procs - 1)
                           ? file_size - start  // Last chunk gets remaining bytes
                           : chunk_size + OVERLAP;
            compress_chunk(input_file, start, length, i);
            exit(0);
        }
    }

    // Parent waits for all children
    for (int i = 0; i < num_procs; i++) {
        wait(NULL);
    }

    // Merge temp files into final output
    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) die("open output");

    char buf[4096], temp_name[32];
    ssize_t r;
    for (int i = 0; i < num_procs; i++) {
        snprintf(temp_name, sizeof(temp_name), "tmp_chunk_%d.txt", i);
        int tmp_fd = open(temp_name, O_RDONLY);
        if (tmp_fd < 0) die("open temp read");

        while ((r = read(tmp_fd, buf, sizeof(buf))) > 0) {
            write(out_fd, buf, r);
        }

        unlink(temp_name);  // delete temp file
    }

    return 0;
}
