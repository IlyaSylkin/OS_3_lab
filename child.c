#include "process.h"

void process_line(char *line, char *shm_buffer, 
                  sem_t *sem_data_ready, sem_t *sem_parent_read) {
    float numbers[100];
    int count = 0;

    sem_wait(sem_parent_read);
    // Парсим числа 
    char *token = strtok(line, " ");
    while (token != NULL && count < 100) {
        numbers[count++] = atof(token);
        token = strtok(NULL, " ");
    }
    
    if (count < 2) {
        snprintf(shm_buffer, BUFFER_SIZE, 
                "Error: line must contain at least 2 numbers\n");
        
        sem_post(sem_data_ready);
        return;
    }
    
    // Формируем вых данные
    int pos = 0;

    pos += snprintf(shm_buffer + pos, BUFFER_SIZE - pos, "Command: ");
    for (int i = 0; i < count && pos < BUFFER_SIZE - 50; i++) {
        pos += snprintf(shm_buffer + pos, BUFFER_SIZE - pos, "%.3f ", numbers[i]);
    }
    pos += snprintf(shm_buffer + pos, BUFFER_SIZE - pos, "\n");
    
    float current_result = numbers[0];
    
    pos += snprintf(shm_buffer + pos, BUFFER_SIZE - pos, 
                   "Division results for %.3f:\n", current_result);
    
    for (int i = 1; i < count && pos < BUFFER_SIZE - 50; i++) {
        float divisor = numbers[i];
        
        if (divisor == 0.0f) {
            strcpy(shm_buffer, "EXIT:division_by_zero");
            sem_post(sem_data_ready);
            exit(EXIT_FAILURE);
        }
        
        float previous_result = current_result;
        current_result = current_result / divisor;
        
        pos += snprintf(shm_buffer + pos, BUFFER_SIZE - pos,
                       "  %.3f / %.3f = %.3f\n", 
                       previous_result, divisor, current_result);
    }
    pos += snprintf(shm_buffer + pos, BUFFER_SIZE - pos, "\n");

    shm_buffer[pos] = '\0';

    sem_post(sem_data_ready);
}

int main(int argc, char *argv[]) { 
    char line_buffer[BUFFER_SIZE * 2];
    size_t buffer_len = 0;
    int bytes_read;
    char *shm_buffer;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: child <filename>\n");
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem_data_ready = sem_open(SEM_DATA_READY, O_RDWR);
    sem_t *sem_parent_read = sem_open(SEM_PARENT_READ, O_RDWR);
    if (sem_data_ready == SEM_FAILED || sem_parent_read == SEM_FAILED) {
        perror("sem_open error in child");
        exit(EXIT_FAILURE);
    }
    
    int file_fd = open(argv[1], O_RDONLY);
    if (file_fd == -1) {
        perror("Error opening file");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        exit(EXIT_FAILURE);
    }

    redirect_fd(file_fd, 0);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open error");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        exit(EXIT_FAILURE);
    }

    shm_buffer = (char *)mmap(NULL, BUFFER_SIZE, 
                             PROT_READ | PROT_WRITE, 
                             MAP_SHARED, shm_fd, 0);
    if (shm_buffer == MAP_FAILED) {
        perror("mmap error");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    // Читаем файл
    while ((bytes_read = read(0, line_buffer + buffer_len, BUFFER_SIZE - buffer_len - 1)) > 0) {
        buffer_len += bytes_read;
        line_buffer[buffer_len] = '\0';
        
        char *start = line_buffer;
        char *end;
        
        // Обраб строку
        while ((end = strchr(start, '\n')) != NULL) {
            *end = '\0';
            
            if (strlen(start) > 0) {
                process_line(start, shm_buffer, sem_data_ready, sem_parent_read);
            }
            
            start = end + 1;
        }
    
    }
    
    if (bytes_read == -1) {
        perror("read from file error");
        sem_close(sem_data_ready);
        sem_close(sem_parent_read);
        munmap(shm_buffer, BUFFER_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    // Сигнализируем о завершении работы
    sem_wait(sem_parent_read);
    strcpy(shm_buffer, "EXIT:process_finished");
    sem_post(sem_data_ready);
    
    // Очищаем ресурсы
    sem_close(sem_data_ready);
    sem_close(sem_parent_read);
    munmap(shm_buffer, BUFFER_SIZE);
    close(shm_fd);
    
    return 0;
}