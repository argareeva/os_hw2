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
int paintings;

void cleanup() {
    sem_close(sem_gallery);         //Закрытие именованного семафора
    sem_unlink("/gallery_sem");     //Удаление именованного семафора
    munmap(shm_gallery, SIZE);      //Отключения отображения разделяемой памяти
}

void sigint_handler(int signum) {
    cleanup();
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Error line command arguments\n");
        return 1;
    }
  
    //Установка обработчика на сигнал SIGINT(при нажатии Ctrl+C), чтобы программа корректно завершила работу
    signal(SIGINT, sigint_handler);
    
    int visitor = atoi(argv[1]);
    
    //Аргументы командной строки
    FILE *input_file = fopen("input.txt", "r");
    FILE *output_file = fopen("output.txt", "w");
    if (!input_file || !output_file) {
        printf("Error opening files\n");
        return 1;
    }

    fscanf(input_file, "%d", &paintings);
    fclose(input_file);
    printf("Number of visitor: %d\nNumber of paintings: %d\n", visitor, paintings);
    fprintf(output_file, "Number of visitor: %d\nNumber of paintings: %d\n", visitor, paintings);

    //Создание области разделяемой памяти
    shm_gallery = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shm_gallery == MAP_FAILED) {
        printf("Error mapping shared memory\n");
        return 1;
    }

    //Открытие/создание именованного семафора
    sem_gallery = sem_open("/gallery_sem", O_CREAT | O_EXCL, 0666, MAX_VISITORS);
    if (sem_gallery == SEM_FAILED) {
        printf("Error creating semaphore\n");
        return 1;
    }

    for (int i = 1; i <= paintings; i++) {
        shm_gallery[i] = 0;
    }

    for (int j = 1; j <= paintings; j++) {
        usleep(rand() % 1000);    //Приостановка текущего процесса на заданное количество микросекунд
        sem_wait(sem_gallery);    //Выполнение операции блокировки семафора
        if (shm_gallery[j] >= MAX_VIEWERS) {
            printf("Visitor %d is waiting for painting %d\n", visitor, j);
            fprintf(output_file, "Visitor %d is waiting for painting %d\n", visitor, j);
            sem_wait(sem_gallery);
        }
        shm_gallery[j]++;
        printf("Visitor %d is watching painting %d (total viewers: %d)\n", visitor, j, shm_gallery[j]);
        fprintf(output_file, "Visitor %d is watching painting %d (total viewers: %d)\n", visitor, j, shm_gallery[j]);
        sem_post(sem_gallery);    //Освобождение семафора
    }
    
    printf("Visitor %d is leaving gallery\n", visitor);
    fprintf(output_file, "Visitor %d is leaving gallery\n", visitor);

    if(shm_gallery[paintings + 1] == 0) {
        sem_post(sem_gallery);    //Освобождение семафора
        cleanup();          //Освобождение ресурсов
    }
    
    fclose(output_file);
    return 0;
}
