#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>

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

void read_file(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file: %s\n", filename);
        exit(1);
    }

    struct index_hdr_s cur_data;
    fread(&cur_data, sizeof(cur_data), 1, file);

    struct index_s *records = (struct index_s *)calloc(cur_data.recsords, sizeof(struct index_s));
    fread(records, sizeof(struct index_s), cur_data.recsords, file);

    for (int i = 0; i < (int)cur_data.recsords; i++)
    {
        printf("%.2ld) %lf\t", records[i].recno, records[i].time_mark);
        if (!((i + 1) % 3))
        {
            puts("");
        }
    }

    puts("");

    free(records);
    fclose(file);
}

int main()
{
    read_file("file");
    return 0;
}