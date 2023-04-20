#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

sem_t *sem_gallery, *sem_visitors;
int *visitors_count;

void cleanup() {
    // Закрываем именованные семафоры
    sem_close(sem_gallery);
    sem_close(sem_visitors);

    // Отсоединяем разделяемую память от процесса
    munmap(visitors_count, sizeof(int));
    // Удаляем именованную разделяемую память
    shm_unlink("/shm_counter");

}

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

    // Открываем именованный семафор для доступа к количеству посетителей в галерее
    sem_gallery = sem_open("/sem_gallery", O_RDWR, 0666, visitors);
    if (sem_gallery == SEM_FAILED) {
        perror("Error creating semaphore sem_gallery\n");
        exit(EXIT_FAILURE);
    }

    // Открываем именованный семафор для доступа к количеству посетителей в галерее
    sem_visitors = sem_open("/sem_visitors", O_RDWR, 0666, visitors);
    if (sem_visitors == SEM_FAILED) {
        perror("Error creating semaphore sem_visitors");
        exit(EXIT_FAILURE);
    }

    // Создаем именованную разделяемую память для хранения счетчика посетителей
    int fd = shm_open("/shm_counter", O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error creating shared memory");
        exit(EXIT_FAILURE);
    }

    //Задаем необходимый размер памяти
    if (ftruncate(fd, sizeof(int)) == -1) {
        perror("Error setting shared memory size");
        exit(EXIT_FAILURE);
    }

    //Присоединяем разделяемую память к процессу
    visitors_count = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (visitors_count == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }

    //Начальное значение счетчика посетителей равно 0
    *visitors_count = 0;

    //Симулируем пребывание посетителей в галерее
    for(int i = 1; i <= visitors; ++i) {
        for (int j = 1; j <= paintings; j++) {
            printf("Visitor %d is viewing painting %d\n", i, j);
            usleep(rand() % 1000);    //Приостановка текущего процесса на заданное количество микросекунд
        }
        printf("Visitor %d is leaving gallery\n", i);     // Отправляем сигнал вахтеру о выходе из галереи
    }
    sem_wait(sem_gallery);
    (*visitors_count)--;
    sem_post(sem_visitors);

    //Освобождение ресурсов
    cleanup();

    //Закрытие файлов
    fclose(input_file);
    fclose(output_file);

    return 0;
}
