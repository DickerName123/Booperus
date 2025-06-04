#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "file_utils.h"

/**
 * tee - duplicates stdin to stdout and a file.
 * @output_file: path to the file where data should be written.
 *
 * Return: 0 on success, -1 on error.
 */
int tee(const char *output_file)
{
    if (!output_file)
        return -1;

    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return -1;

    char buf[4096];
    ssize_t r;
    while ((r = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < r) {
            ssize_t w = write(STDOUT_FILENO, buf + written, r - written);
            if (w == -1) {
                close(fd);
                return -1;
            }
            written += w;
        }
        written = 0;
        while (written < r) {
            ssize_t w = write(fd, buf + written, r - written);
            if (w == -1) {
                close(fd);
                return -1;
            }
            written += w;
        }
    }

    if (r == -1) {
        close(fd);
        return -1;
    }

    if (close(fd) == -1)
        return -1;

    return 0;
}

/**
 * reverse_file - reverse the content of input_file byte by byte.
 *
 * Return: 0 on success, -1 on error.
 */
int reverse_file(const char *input_file, const char *output_file)
{
    if (!input_file || !output_file)
        return -1;

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd == -1)
        return -1;

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd == -1) {
        close(in_fd);
        return -1;
    }

    off_t pos = lseek(in_fd, 0, SEEK_END);
    if (pos == (off_t)-1) {
        close(in_fd);
        close(out_fd);
        return -1;
    }

    while (pos > 0) {
        pos = lseek(in_fd, -1, SEEK_CUR);
        if (pos == (off_t)-1) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
        unsigned char c;
        if (read(in_fd, &c, 1) != 1) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
        if (write(out_fd, &c, 1) != 1) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
        pos = lseek(in_fd, -1, SEEK_CUR);
        if (pos == (off_t)-1) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
    }

    if (close(in_fd) == -1 || close(out_fd) == -1)
        return -1;

    return 0;
}

/**
 * reverse_file_optimized - reverse file using buffer to reduce syscalls.
 *
 * Return: 0 on success, -1 on error.
 */
int reverse_file_optimized(const char *input_file, const char *output_file)
{
    if (!input_file || !output_file)
        return -1;

    int in_fd = open(input_file, O_RDONLY);
    if (in_fd == -1)
        return -1;

    int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd == -1) {
        close(in_fd);
        return -1;
    }

    off_t filesize = lseek(in_fd, 0, SEEK_END);
    if (filesize == (off_t)-1) {
        close(in_fd);
        close(out_fd);
        return -1;
    }

    const size_t BUF_SIZE = 4096;
    unsigned char buf[BUF_SIZE];
    while (filesize > 0) {
        size_t chunk = (filesize > (off_t)BUF_SIZE) ? BUF_SIZE : (size_t)filesize;
        off_t pos = lseek(in_fd, -chunk, SEEK_CUR);
        if (pos == (off_t)-1) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
        ssize_t r = read(in_fd, buf, chunk);
        if (r != (ssize_t)chunk) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
        for (ssize_t i = r - 1; i >= 0; i--) {
            if (write(out_fd, &buf[i], 1) != 1) {
                close(in_fd);
                close(out_fd);
                return -1;
            }
        }
        if (lseek(in_fd, pos, SEEK_SET) == (off_t)-1) {
            close(in_fd);
            close(out_fd);
            return -1;
        }
        filesize -= chunk;
    }

    if (close(in_fd) == -1 || close(out_fd) == -1)
        return -1;

    return 0;
}

