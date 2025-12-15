#include "process.h"

void redirect_fd(int old_fd, int new_fd) {
    if (dup2(old_fd, new_fd) == -1) {
        perror("dup2 error");
        exit(EXIT_FAILURE);
    }
    close(old_fd);
}