#ifndef PROCESS_H
#define PROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <semaphore.h>

#define BUFFER_SIZE 1024
#define SHM_NAME "/lab3_shm"
#define SEM_DATA_READY "/lab3_sem_data"
#define SEM_PARENT_READ "/lab3_sem_parent"

void redirect_fd(int old_fd, int new_fd);

#endif