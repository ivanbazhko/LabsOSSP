#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

#define MAX_RECORDS 15
#define RECORD_SIZE sizeof(struct student)

struct student
{
    char name[80];
    char city[80];
    uint8_t semester;
};

struct current
{
    struct student record;
    int number;
} current_rec;

void print_record(struct student *record)
{
    printf("Имя: %s\n", record->name);
    printf("Город: %s\n", record->city);
    printf("Семестр: %d\n", record->semester);
    printf("----------------------------\n");
}

void read_record(int file, int record_number, struct student *record)
{
    lseek(file, record_number * RECORD_SIZE, SEEK_SET);
    read(file, record, RECORD_SIZE);
}

void write_record(int file, int record_number, struct student *record)
{
    lseek(file, record_number * RECORD_SIZE, SEEK_SET);
    write(file, record, RECORD_SIZE);
}

void lock_record(int file, int record_number) // блокировка записи в файле для записи
{
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = record_number * RECORD_SIZE;
    lock.l_len = RECORD_SIZE;

    fcntl(file, F_SETLK, &lock);

    int flags = fcntl(file, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(file, F_SETFL, flags);
}

void unlock_record(int file, int record_number) // разблокировка записи в файле
{
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = record_number * RECORD_SIZE;
    lock.l_len = RECORD_SIZE;

    fcntl(file, F_SETLK, &lock);

    int flags = fcntl(file, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(file, F_SETFL, flags);
}

int main()
{
    printf("LST - вывести содержимое файла\n");
    printf("GET(XX) - получить запись по номеру XX\n");
    printf("MOD - изменить запись\n");
    printf("q - выйти\n");
    srand((unsigned int)getpid());
    int myfile = open("file", O_RDWR | O_CREAT);
    struct student records[MAX_RECORDS];
    struct student record;

    if (myfile == -1)
    {
        perror("Error opening file");
        return 1;
    }

    char input[10];
    while (1)
    {
        fgets(input, sizeof(input), stdin);
        if (strstr(input, "q"))
        {
            break;
        }
        else if (strstr(input, "LST")) // отображение содержимого файла
        {

            printf("Содержимое файла:\n");
            for (int i = 0; i < MAX_RECORDS; i++)
            {
                struct student record;
                read_record(myfile, i, &record);
                printf("%d:\n", i);
                print_record(&record);
            }
        }
        else if (strstr(input, "GET(")) // получение записи по порядковомц номеру
        {
            int record_number = atoi(&input[4]);
            if (record_number >= MAX_RECORDS)
                continue;
            struct student record;
            read_record(myfile, record_number, &record);
            printf("Полученная запись:\n");
            print_record(&record);

            current_rec.record = record;
            current_rec.number = record_number;
        }
        else if (strstr(input, "PUT")) // запись структуры в файл
        {
            write_record(myfile, current_rec.number, &record);
            unlock_record(myfile, current_rec.number);
            puts("Запись Сохранена");
            current_rec.record = record;
        }
        else if (strstr(input, "MOD")) // модификация записи
        {
            printf("Изменение записи номер %d:\n", current_rec.number);
            printf("Введите имя: \n");
            scanf("%80s", record.name);
            printf("Введите город: \n");
            scanf("%80s", record.city);
            printf("Введите семестр: \n");
            scanf("%hhu", &record.semester);

            struct student mod_stud;
            mod_stud = current_rec.record;
            lock_record(myfile, current_rec.number);
            struct student now_rec;
            read_record(myfile, current_rec.number, &now_rec);

            if (memcmp(&now_rec, &mod_stud, sizeof(struct student)) != 0) // проверка, была ли изменена запись
            {
                puts("Запись в файле изменена другим пользователем!");
                unlock_record(myfile, current_rec.number);
                current_rec.record = now_rec;
                puts("Текущая Запись:\n");
                printf("Имя: %s\n", current_rec.record.name);
                printf("Город: %s\n", current_rec.record.city);
                printf("Семестр: %d\n", current_rec.record.semester);
                printf("----------------------------\n");
                continue;
            }

            puts("Введите \"PUT\" чтобы продолжить запись");
        }
    }

    close(myfile);

    return 0;
}