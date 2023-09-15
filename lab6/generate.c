#define _GNU_SOURCE

#define FILE_NAME "file"
#define MAX 60190
#define MIN 15020

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include <sys/types.h>

struct index_s
{
    double time_mark; // временная метка (модифицированная юлианская дата)
    uint64_t recno;   // первичный индекс в таблице БД
};

struct index_hdr_s
{
    uint64_t recsords;   // количество записей
    struct index_s *idx; // массив записей в количестве records
};

struct index_hdr_s *buffer;

void generate(int buffer_size)
{
    FILE *file = fopen(FILE_NAME, "wb");
    struct index_hdr_s data;
    data.recsords = buffer_size / 16;
    data.idx = (struct index_s *)calloc(data.recsords, sizeof(struct index_s));
    fwrite(&data.recsords, sizeof(uint64_t), 1, file);
    fwrite(&data.idx, sizeof(struct index_s *), 1, file);

    for (int i = 0; i < buffer_size / 16; i++)
    {
        if (!(i % 3))
        {
            puts("");
        }
        struct index_s tmp;
        tmp.recno = i + 1;
        tmp.time_mark = ((double)rand() / RAND_MAX) * (MAX - MIN) + MIN;
        data.idx[i] = tmp;

        printf("%.2ld) %lf\t", tmp.recno, tmp.time_mark);
        fwrite(&tmp, sizeof(struct index_s), 1, file);
    }
    puts("");
    const char end = '\0';
    fwrite(&end, 1, 32, file);
    fclose(file);
}

int main(int argc, char *agrv[])
{
    if (argc != 2 || atoi(agrv[1]) % 256)
    {
        puts("Размер должен быть кратен 256");
        return 1;
    }
    srand((unsigned int)getpid());
    generate(atoi(agrv[1]));
    return 0;
}