#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#define MAX_VISITORS 10

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

    int visitors, paintings;
    fscanf(input_file, "%d %d", &visitors, &paintings);
    printf("Number of visitors: %d\nNumber of paintings: %d\n", visitors, paintings);

    // Создаем разделяемую память
    int fd = shm_open("/gallery_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем размер памяти
    if (ftruncate(fd, sizeof(int)) == -1) {
        printf("Error setting shared memory size\n");
        return 1;
    }

    // Отображаем память в адресное пространство
    int* visitors_count = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (visitors_count == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Инициализируем семафоры
    sem_t* waits_sem = sem_open("/gallery_waits_sem", O_CREAT, 0666, 0);
    sem_t* mutex_sem = sem_open("/gallery_mutex_sem", O_CREAT, 0666, 1);
    if (waits_sem == SEM_FAILED || mutex_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // Инициализируем счетчик посетителей
    *visitors_count = 0;

    for (int i = 1; i <= visitors; i++) {
        sem_wait(waits_sem);    //Ожидаем появления нового посетителя
        sem_wait(mutex_sem);    //Получаем доступ к счетчику посетителей
        int count = ++(*visitors_count);
        sem_post(mutex_sem);    //Освобождение семафора
        if (count > MAX_VISITORS) {     //Проверяем, не превышено ли количество посетителей
            printf("Visitor %d is waiting for painting \n", i);
            fprintf(output_file, "Visitor %d is waiting for painting \n", i);
            sem_wait(waits_sem);    //Отправляем нового посетителя ждать
        } else {
            printf("New visitor is watching paintings (total viewers: %d)\n", count);
            fprintf(output_file, "New visitor is watching paintings (total viewers: %d)\n", count);
        }
    }

    return 0;
}
