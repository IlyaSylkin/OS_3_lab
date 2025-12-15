#include "process.h"

int main() {
    pid_t pid;
    char filename[100];
    sem_t *sem_data_ready, *sem_parent_read;
    char *shm_buffer; 
    
    //как открыть. кт может открыть
    sem_data_ready = sem_open(SEM_DATA_READY, O_CREAT | O_RDWR, 0666, 0);
    sem_parent_read = sem_open(SEM_PARENT_READ, O_CREAT | O_RDWR, 0666, 1);
    if (sem_data_ready == SEM_FAILED || sem_parent_read == SEM_FAILED) {
        perror("sem_open error");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open error");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);

        // sem_unlink(const char *name) - УДАЛЕНИЕ ИЗ СИСТЕМЫ
        sem_unlink(SEM_DATA_READY);
        sem_unlink(SEM_PARENT_READ);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
        perror("ftruncate error");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        sem_unlink(SEM_DATA_READY);
        sem_unlink(SEM_PARENT_READ);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    
    // Проецируем shared memory как char массив
    shm_buffer = (char *)mmap(
        NULL,                     
        BUFFER_SIZE,              
        PROT_READ | PROT_WRITE,   
        MAP_SHARED,  // видны другим             
        shm_fd,                   
        0                         
);
    if (shm_buffer == MAP_FAILED) {
        perror("mmap error");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        sem_unlink(SEM_DATA_READY);
        sem_unlink(SEM_PARENT_READ);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    memset(shm_buffer, 0, BUFFER_SIZE);

    printf("Enter filename: ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        fprintf(stderr, "Error reading filename\n");
        exit(EXIT_FAILURE);
    }
    filename[strcspn(filename, "\n")] = '\0';
    
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork error");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        close(file_fd);

        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        munmap(shm_buffer, BUFFER_SIZE);
        close(shm_fd);
        
        execl("./child", "child", filename, NULL);
        perror("execl error");
        exit(EXIT_FAILURE);
    } else {

        close(file_fd);

        while (1) {
            // Ждем, пока данные готовы
            sem_wait(sem_data_ready);

            if (strcmp(shm_buffer, "EXIT:division_by_zero") == 0) {
                printf("\nDivision by zero detected! Terminating.\n");
                break;
            }
            
            if (strcmp(shm_buffer, "EXIT:process_finished") == 0) {
                // Все обработано
                break;
            }

            printf("%s", shm_buffer);

            memset(shm_buffer, 0, BUFFER_SIZE);
            
            // Сигнал что прочитали
            sem_post(sem_parent_read);
        }
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child process exited with code: %d\n", WEXITSTATUS(status));
        }

        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        sem_unlink(SEM_DATA_READY);
        sem_unlink(SEM_PARENT_READ);
        munmap(shm_buffer, BUFFER_SIZE);
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }
    
    return 0;
}

// ЦИКЛ ОБРАБОТКИ:
// 1. Ребенок: sem_wait(sem_parent_read) - ждет, пока родитель прочитает
// 2. Ребенок: пишет данные в buffer
// 3. Ребенок: sem_post(sem_data_ready) - сигнал родителю
// 4. Родитель: sem_wait(sem_data_ready) - ждет данные
// 5. Родитель: читает buffer, выводит
// 6. Родитель: sem_post(sem_parent_read) - сигнал ребенку
// 7. Переход к шагу 1

