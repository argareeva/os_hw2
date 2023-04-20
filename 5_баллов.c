#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>
#define MAX_VISITORS 100
#define MAX_VIEWERS 10
#define MAX_PAINTINGS 100
#define SIZE sizeof(int[MAX_PAINTINGS])

sem_t *sem_gallery;
int *shm_gallery;
int visitors, paintings;

void cleanup() {
    sem_destroy(sem_gallery);       //Удаление неименованного семафора
    munmap(shm_gallery, SIZE);      //Отключения отображения разделяемой памяти
}

void sigint_handler(int signum) {
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error line command arguments\n");
        return 1;
    }
  
    //Установка обработчика на сигнал SIGINT(при нажатии Ctrl+C), чтобы программа корректно завершила работу
    signal(SIGINT, sigint_handler);

    //Аргументы командной строки  
    FILE *input_file = fopen(argv[1], "r");
    FILE *output_file = fopen(argv[2], "w");
    if (!input_file || !output_file) {
        printf("Error opening files\n");
        return 1;
    }

    fscanf(input_file, "%d %d", &visitors, &paintings);
    printf("Number of visitors: %d\nNumber of paintings: %d\n", visitors, paintings);

    //Создание анонимной области разделяемой памяти для семафоров
    sem_gallery = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem_gallery == MAP_FAILED) {
        printf("Error mapping semaphore memory\n");
        return 1;
    }

    //Инициализация неименованного семафора
    if (sem_init(sem_gallery, 1, MAX_VISITORS) == -1) {
        printf("Error initializing semaphore\n");
        return 1;
    }

    //Создание объекта разделяемой памяти с именем "/gallery_shm" (файловый дескриптор)
    int fd = shm_open("/gallery_shm", O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        printf("Error creating shared memory\n");
        return 1;
    }
    
     //Устанавливаем размер файла
    if (ftruncate(fd, SIZE) == -1) {
        printf("Error setting shared memory size\n");
        return 1;
    }

    //Создание области разделяемой памяти, при этом связываем выделенное пространство с файловым дескриптором
    shm_gallery = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_gallery == MAP_FAILED) {
        printf("Error mapping shared memory\n");
        return 1;
    }

    close(fd);

    for (int i = 1; i <= paintings; i++) {
        shm_gallery[i] = 0;
    }

    pid_t pid;
    for (int i = 1; i <= visitors; i++) {
        pid = fork();
        if (pid < 0) {    //Дочерний процесс
            printf("Error creating child process\n");
            return 1;
        } else if (pid == 0) {    //Родительский процесс
            for (int j = 1; j <= paintings; j++) {
                usleep(rand() % 1000);    //Приостановка текущего процесса на заданное количество микросекунд
                sem_wait(sem_gallery);    //Выполнение операции блокировки семафора
                if (shm_gallery[j] >= MAX_VIEWERS) {
                    printf("Visitor %d waiting for painting %d\n", i, j);
                    fprintf(output_file, "Visitor %d waiting for painting %d\n", i, j);
                    sem_wait(sem_gallery);
                }
                shm_gallery[j]++;
                printf("Visitor %d is watching painting %d (total viewers: %d)\n", i, j, shm_gallery[j]);
                fprintf(output_file, "Visitor %d viewed painting %d (total viewers: %d)\n", i, j, shm_gallery[j]);
                sem_post(sem_gallery);    //Освобождение семафора
            }
            exit(0);
        }
    }

    //Завершение всех дочерних процессов
    for (int i = 1; i <= visitors; i++) {
        wait(NULL);
    }

    //Освобождение ресурсов
    cleanup();
  
    //Закрытие файлов
    fclose(input_file);
    fclose(output_file);

    return 0;
}
