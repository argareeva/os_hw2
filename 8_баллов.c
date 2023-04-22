#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#define MAX_VISITORS 100
#define MAX_VIEWERS 10
#define MAX_PAINTINGS 100
#define SIZE sizeof(int[MAX_PAINTINGS])
#define SEM_KEY 1234
#define SHM_KEY 5678

int *shm_gallery;
int sem_id;     //идентификатор семафора 
int shm_id;     //идентификатор разделяемой памяти
int paintings;

void cleanup() {
    semctl(sem_id, 0, IPC_RMID, 0);     //Удаление семафора с идентификатором sem_id
    shmdt(shm_gallery);                 //Отсоединяем общую память от адресного пространства текущего процесса
    shmctl(shm_id, IPC_RMID, 0);        //Удаление разделяемой памяти с идентификатором shm_id
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
    
    //Аргументы командной строки  
    int visitor = atoi(argv[1]);
    
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

    //Создание новой области разделяемой памяти (или получение уже существующей по заданному ключу)
    shm_id = shmget(SHM_KEY, SIZE, IPC_CREAT | 0666);
    if (shm_id == -1) {
        printf("Error creating shared memory\n");
        return 1;
    }

    //Присоединение(подключение) области разделяемой памяти к адресному пространству текущего процесса
    shm_gallery = shmat(shm_id, NULL, 0);     //shm_id - идентификатор разделяемой памяти, которую мы хотим присоединить
    if (shm_gallery == (int *) -1) {
        printf("Error attaching shared memory\n");
        return 1;
    }

    //Создание нового семафора (или получение идентификатора существующего семафора по заданному ключу) 
    sem_id = semget(SEM_KEY, MAX_PAINTINGS, IPC_CREAT | 0666);
    if (sem_id == -1) {
        printf("Error creating semaphore set\n");
        return 1;
    }

    for (int i = 1; i <= paintings; i++) {
        shm_gallery[i] = 0;
        semctl(sem_id, i, SETVAL, MAX_VISITORS);    //устанавливаем начальное значение семафоров = MAX_VISITORS в массиве
    }

    for (int j = 1; j <= paintings; j++) {
        usleep(rand() % 1000);    //Приостановка текущего процесса на заданное количество микросекунд
        if (shm_gallery[j] >= MAX_VIEWERS) {
            printf("Visitor %d is waiting for painting %d\n", visitor, j);
            fprintf(output_file, "Visitor %d is waiting for painting %d\n", visitor, j);
            usleep(rand() % 100000); 
        }
        shm_gallery[j]++;
        printf("Visitor %d watched painting %d (total viewers: %d)\n", visitor, j, shm_gallery[j]);
        fprintf(output_file, "Visitor %d watched painting %d (total viewers: %d)\n", visitor, j, shm_gallery[j]);
    }

    printf("Visitor %d is leaving gallery\n", visitor);
    fprintf(output_file, "Visitor %d is leaving gallery\n", visitor);

    if(shm_gallery[paintings + 1] == 0) {
        cleanup();          //Освобождение ресурсов
    }
    
    fclose(output_file);
    
    return 0;
}
