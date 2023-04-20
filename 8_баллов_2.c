#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#define SEM_VISITORS_KEY 1234
#define SEM_GALLERY_KEY 5678
#define SHM_COUNTER_KEY 7890

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error line command arguments\n");
        return 1;
    }
    
    //Аргументы командной строки  
    FILE *input_file = fopen(argv[1], "r");
    FILE *output_file = fopen(argv[2], "w");
    if (!input_file || !output_file) {
        printf("Error opening files\n");
        return 1;
    }

    fscanf(input_file, "%d %d", &visitors, &paintings);
    printf("Number of visitors: %d\nNumber of paintings: %d\n", visitors, paintings);
    fprintf(output_file, "Number of visitors: %d\nNumber of paintings: %d\n", visitors, paintings);
    
    // Получаем доступ к семафору для доступа к количеству посетителей в галерее
    int sem_gallery = semget(SEM_GALLERY_KEY, 1, 0666);
    if (sem_gallery == -1) {
        perror("Error creating semaphore sem_gallery\n");
        return 1;
    }

    // Получаем доступ к семафору для доступа к количеству посетителей в галерее
    int sem_visitors = semget(SEM_VISITORS_KEY, 1, 0666);
    if (sem_visitors == -1) {
        perror("Error creating semaphore sem_visitors\n");
        return 1;
    }

    // Получаем доступ к разделяемой памяти для хранения счетчика посетителей
    int shm_id = shmget(SHM_COUNTER_KEY, sizeof(int), 0666);
    if (shm_id == -1) {
        perror("Error creating shared memory\n");
        return 1;
    }

    // Присоединяем разделяемую память к процессу
    int *visitors_counter = shmat(shm_id, NULL, 0);
    if (visitors_counter == (int *) -1) {
        perror("Error attaching shared memory\n");
        return 1;
    }

    // Задаем seed для функции rand()
    srand(getpid());

    // Симулируем пребывание посетителей в галерее
    for (int i = 1; i <= paintings; i++) {
        // Случайный выбор времени пребывания у текущей картины
        int view_time = rand() % 7 + 1;
        // Симуляция пребывания у текущей картины
        printf("Visitor %d is watching painting %d for %d sec\n", getpid(), i, view_time);
        fprintf(output_file, "Visitor %d is watching painting %d for %d sec\n", getpid(), i, view_time);
        sleep(view_time);

        // Случайная задержка перед переходом к следующей картине
        int wait_time = rand() % 7 + 1;
        printf("Visitor %d is waiting %d sec before moving to the different picture\n", getpid(), wait_time);
        fprintf(output_file, "Visitor %d is waiting %d sec before moving to the different picture\n", getpid(), wait_time);
        sleep(wait_time);
    }

    // Уменьшаем количество посетителей в галерее
    struct sembuf sops = { 0, -1, 0 };
    if (semop(sem_visitors, &sops, 1) == -1) {
        perror("Error decreasing the amount of sem_visitors");
        return 1;
    }

    // Уменьшаем количество посетителей в галерее, которые смотрят картину
    if (semop(sem_gallery, &sops, 1) == -1) {
        perror("Error decreasing the amount of sem_gallery");
        return 1;
    }

    // Увеличиваем счетчик посетителей в разделяемой памяти
    (*visitors_counter)++;

    // Отсоединяем разделяемую память от процесса
    shmdt(visitors_counter);

    return 0;
}
