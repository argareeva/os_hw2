#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#define MAX_VISITORS 50
#define SEM_VISITORS 0
#define SEM_KEY 1234
#define SHM_KEY 5678
#define SEM_GALLERY 1
#define SHM_COUNTER 2

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
    
    // Создаем разделяемую память для хранения счетчика посетителей
    int shm_id = shmget(SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Error creating shared memory\n");
        return 1;
    }

    // Присоединяем разделяемую память к процессу
    int *visitors_counter = shmat(shm_id, NULL, 0);
    if (visitors_counter == (int*) -1) {
        perror("Error attaching shared memory\n");
        return 1;
    }

    // Начальное значение счетчика посетителей равно 0
    *visitors_counter = 0;

    // Создаем два семафора - для количества посетителей и для доступа к галерее
    int sem_id = semget(SEM_KEY, 2, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Ошибка создания семафоров");
        return 1;
    }

    // Инициализируем семафоры
    semctl(sem_id, SEM_VISITORS, SETVAL, 0);
    semctl(sem_id, SEM_GALLERY, SETVAL, visitors);

    return 0;
}
