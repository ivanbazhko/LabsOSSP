#define _GNU_SOURCE

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
#include <semaphore.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>

struct index_record
{
    double time_mark; // временная метка (модифицированная юлианская дата)
    uint64_t recno;   // первичный индекс в таблице БД
};

int pagesize;
int memsize;
int blocks;
int threads;
int block_size;
char *filename;
int print_break;

pthread_barrier_t mybarrier;
pthread_mutex_t main_mutex;
struct index_record *buffer;
struct index_record *result_buf;
uint64_t count = 0;
int start = 0;
void *address;
int *block_status;

void merge(int argument);

int is_sub_2(int number)
{
    int sub = 1;
    while (number >= sub)
    {
        if (number == sub)
        {
            return 1;
        }
        else
        {
            sub *= 2;
        }
    }
    return 0;
}

int compare(const void *a, const void *b)
{
    struct index_record num1 = *((struct index_record *)a);
    struct index_record num2 = *((struct index_record *)b);
    if (num1.time_mark < num2.time_mark)
        return -1;
    else if (num1.time_mark > num2.time_mark)
        return 1;
    else
        return 0;
}

int next_block() // переход к сортировке следующего несортированного блока
{
    int number = -1;
    pthread_mutex_lock(&main_mutex);

    for (int i = 0; i < blocks; i++)
    {
        if (!block_status[i])
        {
            block_status[i] = 1;
            number = i;
            break;
        }
    }
    pthread_mutex_unlock(&main_mutex);
    return number;
}

int merger_blocks(int current)
{
    int number = -1;

    pthread_mutex_lock(&main_mutex);

    for (int i = 0; i < blocks / current; i++)
    {
        if (block_status[i])
        {
            block_status[i] = 0;
            number = i;
            break;
        }
    }

    pthread_mutex_unlock(&main_mutex);

    return number;
}

void *sort(void *arg)
{
    int argument = *(int *)arg;
    int number = argument;

    pthread_barrier_wait(&mybarrier);

    while (number >= 0) // сортировка блока
    {
        struct index_record *block = calloc(block_size, sizeof(struct index_record));
        for (int i = 0; i < block_size; i++)
        {
            block[i] = buffer[i + block_size * number];
        }

        qsort(block, block_size, sizeof(struct index_record), compare);

        for (int i = 0; i < block_size; i++)
        {
            buffer[i + block_size * number] = block[i];
        }

        number = next_block(); // переход к сортировке следующего несортированного блока
        if (number == -1)
        {
            break;
            free(block);
        }
        free(block);
    }

    pthread_barrier_wait(&mybarrier);

    if (argument == 0) // вывод отсортированных блоков одним потоком
    {
        puts("После Сортировки Блоков:\n");
        for (int i = 0; i < (int)count; i++)
        {
            printf("%.2ld) %lf\n", buffer[i].recno, buffer[i].time_mark);
            if (((i + 1) % print_break) == 0)
            {
                printf("\n");
            }
        }
    }

    merge(argument);
    pthread_exit(NULL);
}

void merge(int argument) // слияние отсортированных блоков
{
    int current_joined = 1;
    int number = 0;
    int new_block_size = block_size;
    while (current_joined != (int)blocks)
    {
        number = 0;
        current_joined *= 2;
        new_block_size = new_block_size * 2;

        pthread_barrier_wait(&mybarrier);

        while (number >= 0)
        {
            number = merger_blocks(current_joined);
            if (number == -1)
            {
                break;
            }

            struct index_record *block = calloc(new_block_size, sizeof(struct index_record));
            for (int i = 0; i < new_block_size; i++)
            {
                block[i] = buffer[i + new_block_size * number];
            }

            int i = 0, j = new_block_size / 2, k = 0;
            for (; (i < new_block_size / 2) && (j < new_block_size);)
            {
                if (block[i].time_mark > block[j].time_mark)
                {
                    buffer[k + new_block_size * number] = block[j];
                    j++;
                }
                else
                {
                    buffer[k + new_block_size * number] = block[i];
                    i++;
                }
                k++;
            }
            while (j < new_block_size)
            {
                buffer[k + new_block_size * number] = block[j++];
                k++;
            }
            while (i < new_block_size / 2)
            {
                buffer[k + new_block_size * number] = block[i++];
                k++;
            }

            free(block);
        }

        pthread_barrier_wait(&mybarrier);

        for (int i = 0; i < blocks; i++)
        {
            block_status[i] = 1;
        }
    }

    pthread_barrier_wait(&mybarrier);
}

int main(int argc, char *argv[])
{
    if (argc > 5)
    {
        puts("Слишком много параметров!");
        return 1;
    }
    if (argc < 5)
    {
        puts("Слишком мало параметров!");
        return 1;
    }
    pagesize = getpagesize();
    memsize = atoi(argv[1]);
    blocks = atoi(argv[2]);
    threads = atoi(argv[3]);
    filename = calloc(1, strlen(argv[4]));
    strcpy(filename, argv[4]);

    if (memsize % pagesize != 0)
    {
        printf("Размер должен быть кратен %d\n", pagesize);
        free(filename);
        return 0;
    }
    if (!is_sub_2(blocks) || blocks > 256)
    {
        puts("Количество блоков не должно быть больше 256");
        free(filename);
        return 0;
    }
    if (threads > 100 || threads < 4 || blocks < threads)
    {
        puts("Количество потоков должно быть в пределе от 4 до 100 и не больше, чем количество блоков");
        free(filename);
        return 0;
    }

    int file;
    file = open(filename, O_RDWR);
    if (file < 0)
    {
        printf("Ошибка открытия файла %s\n", filename);
        free(filename);
        return 0;
    }

    srand((unsigned int)getpid());

    pthread_mutex_init(&main_mutex, NULL);
    pthread_barrier_init(&mybarrier, NULL, threads);

    block_status = (int *)calloc(blocks, sizeof(int));

    uint64_t fromfilenum;
    int end = 0;
    read(file, &fromfilenum, 8);
    int rec_in_file = fromfilenum;
    int file_iterations = 0;

    print_break = memsize / 16 / blocks;

    if (memsize > (int)(16 * rec_in_file))
    {
        printf("Размер %d слишком большой\n", memsize);
        free(filename);
        return 0;
    };

    struct index_record *res_array;
    struct index_record *res_merged_array;
    res_array = (struct index_record *)calloc(rec_in_file, sizeof(struct index_record));
    res_merged_array = (struct index_record *)calloc(rec_in_file, sizeof(struct index_record));
    int received = 0;

    while (count <= fromfilenum)
    {
        file_iterations++;
        count = memsize / 16;
        address = mmap(NULL, memsize + 1, PROT_READ | PROT_WRITE, MAP_SHARED, file, memsize * end);

        buffer = calloc(1, memsize);

        memcpy(buffer, (char *)(address) + 16, memsize);

        block_size = count / blocks;

        for (int i = 0; i < threads; i++)
        {
            block_status[i] = 1;
        }
        for (int i = threads; i < blocks; i++)
        {
            block_status[i] = 0;
        }

        pthread_t *thread = calloc(threads, sizeof(pthread_t));

        int *threads_nums = calloc(threads, sizeof(int));
        for (int i = 0; i < threads; i++)
        {
            threads_nums[i] = i;
            (void)pthread_create(&thread[i], NULL, sort, &threads_nums[i]);
        }

        for (int i = 0; i < threads; i++)
        {
            pthread_join(thread[i], 0);
        }

        for (int i = 0; i < (int)count; i++)
        {
            res_array[received].recno = buffer[i].recno;
            res_array[received++].time_mark = buffer[i].time_mark;
        }

        for (int i = 0; i < (int)count; i++)
        {
            memcpy((char *)address + 16 + i * 16, &buffer[i], 16);
        }

        free(threads_nums);
        free(thread);
        end++;
        fromfilenum -= count;

        munmap(address, memsize);
        free(buffer);
    }

    printf("После Слияния Блоков:\n");

    received = 0;
    int *block_smallest = (int *)calloc(file_iterations, sizeof(int));
    double min_val = 0;
    int watch = 0;
    int flag = 1;

    if (file_iterations > 1)
    {
        do // слияние отсортированных блоков
        {
            for (int y = 0; y < file_iterations; y++)
            {
                if (block_smallest[y] != -1)
                {
                    if (min_val == 0)
                    {
                        min_val = res_array[(memsize / 16) * y + block_smallest[y]].time_mark;
                        watch = y;
                        continue;
                    }
                    if (res_array[(memsize / 16) * y + block_smallest[y]].time_mark < min_val)
                    {
                        min_val = res_array[(memsize / 16) * y + block_smallest[y]].time_mark;
                        watch = y;
                        continue;
                    }
                }
                else {
                    continue;
                };
            }
            res_merged_array[received].recno = res_array[(memsize / 16) * watch + block_smallest[watch]].recno;
            res_merged_array[received].time_mark = min_val;
            block_smallest[watch]++;
            received++;
            if (block_smallest[watch] >= (memsize / 16))
            {
                block_smallest[watch] = -1;
            };
            min_val = 0;
            watch = 0;
            flag = 0;
            for (int y = 0; y < file_iterations; y++)
            {
                if (block_smallest[y] != -1)
                {
                    flag = 1;
                };
            }
        } while (flag);

        address = mmap(NULL, memsize * file_iterations + 1, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
        for (int i = 0; i < rec_in_file; i++)
        {
            memcpy((char *)address + 16 + i * 16, &res_merged_array[i], 16);
        }
        munmap(address, memsize * file_iterations);

        printf("\n");
        for (int i = 0; i < rec_in_file; i++)
        {
            printf("%.2ld) %lf\t", res_merged_array[i].recno, res_merged_array[i].time_mark);
            if (!((i + 1) % 3))
            {
                puts("");
            }
        } 
    }
    else
    {

        for (int i = 0; i < rec_in_file; i++)
        {
            printf("%.2ld) %lf\t", res_array[i].recno, res_array[i].time_mark);
            if (!((i + 1) % 3))
            {
                puts("");
            }
        }
    }
    printf("\n");

    free(res_array);
    free(res_merged_array);
    free(block_smallest);
    close(file);
    pthread_barrier_destroy(&mybarrier);
    pthread_mutex_destroy(&main_mutex);

    return 0;
}