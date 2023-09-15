#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>

#include <signal.h>
#include <errno.h>

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define DATA_MAX (((256 + 3) / 4) * 4)
#define MAX_LEN 100

#define SHM_OBJECT "/queue"
#define MUTEX "mutex"
#define FREE_SPACE "free_space"
#define ITEMS "items"

sem_t queue_left;
sem_t queue_occup;
pthread_mutex_t thread_queue_mutex;
size_t queue_len = 10;

static int cons_status[MAX_LEN];
static int pros_status[MAX_LEN];

pid_t prod_array[MAX_LEN];
size_t prod_number;
int wcounter = 0;

pid_t cons_array[MAX_LEN];
size_t cons_number;

struct message // сообщение
{
    int8_t type;
    uint16_t hash;
    uint8_t size;
    char data[DATA_MAX];
};

struct queue // очередь
{
    size_t add_num;
    size_t del_num;
    size_t current_num;
    size_t head;
    size_t tail;
    struct message *buffer;
};

struct queue *myqueue;
static pid_t parent;

///////////////////////////////////////////////////////////////////////

uint16_t hash(struct message *msg);
void queue_init();
size_t put_message(struct message *message);
size_t get_message(struct message *message);
void initialize();
void clear_proc();
void delete_producer();
void generate_message(struct message *message);
void delete_consumer();
void consume_message(struct message *message);

///////////////////////////////////////////////////////////////////////

uint16_t hash(struct message *msg)
{
    unsigned long hash = 1234;

    for (int i = 0; i < msg->size + 4; ++i)
    {
        hash = ((hash << 5) + hash) + i;
    }

    return (uint16_t)hash;
}

void queue_init() // инициализация очереди
{
    myqueue = malloc(sizeof(struct queue));
    myqueue->add_num = 0;
    myqueue->del_num = 0;
    myqueue->current_num = 0;
    myqueue->head = 0;
    myqueue->tail = 0;
    myqueue->buffer = malloc(queue_len * sizeof(struct message));
    //  Инициализация семафора queue_left
    sem_init(&queue_left, 0, queue_len);
    // Инициализация семафора queue_occup
    sem_init(&queue_occup, 0, 0);
}

size_t put_message(struct message *message) // добавление сообщения в очередь
{
    if (myqueue->current_num == queue_len)
    {
        fputs("Переполнение очереди\n", stderr);
        exit(EXIT_FAILURE);
    }

    if (myqueue->head == queue_len)
    {
        myqueue->head = 0;
    }
    myqueue->buffer[myqueue->head] = *message;
    ++myqueue->head;
    ++myqueue->current_num;
    return ++myqueue->add_num;
}

size_t get_message(struct message *message) // удаление сообщения из очереди
{
    if (myqueue->current_num == 0)
    {
        fputs("Недостаточно сообщений\n", stderr);
        exit(EXIT_FAILURE);
    }

    *message = myqueue->buffer[myqueue->tail];
    ++myqueue->tail;

    if (myqueue->tail == queue_len)
    {
        myqueue->tail = 0;
    }

    --myqueue->current_num;

    return ++myqueue->del_num;
}

void print_queue() // Вывод очереди на экран
{
    pthread_mutex_lock(&thread_queue_mutex);
    if (myqueue->current_num == 0)
        printf("Очередь пуста!\n");
    else if (myqueue->head > myqueue->tail)
    {
        for (size_t j = myqueue->tail; j < myqueue->head; j++)
        {
            printf("сообщение: тип=%X, размер=%X, hash=%X\n",
                   myqueue->buffer[j].type, myqueue->buffer[j].size, myqueue->buffer[j].hash);
        }
    }
    else
    {
        size_t j = myqueue->tail;
        if (myqueue->tail == myqueue->head)
        {
            printf("сообщение: тип=%X, размер=%X, hash=%X\n",
                   myqueue->buffer[j].type, myqueue->buffer[j].size, myqueue->buffer[j].hash);
            j++;
            if (j == queue_len)
                j = 0;
        }
        for (j; j != myqueue->head; j++)
        {
            if (j == queue_len)
                j = 0;
            printf("сообщение: тип=%X, размер=%X, hash=%X\n",
                   myqueue->buffer[j].type, myqueue->buffer[j].size, myqueue->buffer[j].hash);
        }
    }
    pthread_mutex_unlock(&thread_queue_mutex);
}

///////////////////////////////////////////////////////////////////////

void *producer_thread(void *args)
{
    int number = *((int *)args);
    printf("Создан производитель N %d\n", number);
    struct message msg;
    size_t add_count_local;
    while (1)
    {
        if (pros_status[number - 1] == 0)
            pthread_exit(NULL);
        generate_message(&msg);
        sem_wait(&queue_left);
        pthread_mutex_lock(&thread_queue_mutex);
        add_count_local = put_message(&msg);
        pthread_mutex_unlock(&thread_queue_mutex);
        sem_post(&queue_occup);
        printf("P %d: сообщение создано: тип=%X, размер=%X, hash=%X, всего создано:%zu\n",
               number, msg.type, msg.size, msg.hash, add_count_local);
        for (int u = 0; u < 10; u++)
        {
            sleep(1);
        }
    }
}

void generate_message(struct message *message) // создание сообщения производителем
{
    size_t value = rand() % 257;
    if (value == 256)
    {
        message->type = -1;
    }
    else
    {
        message->type = 0;
        message->size = (uint8_t)value;
    }

    for (size_t i = 0; i < value; ++i)
    {
        message->data[i] = (char)(rand() % 256);
    }

    message->hash = 0;
    message->hash = hash(message);
}

///////////////////////////////////////////////////////////////////////

void *consumer_thread(void *args)
{
    int number = *((int *)args);
    printf("Создан потребитель N %d\n", number);
    struct message msg;
    size_t extract_count_local;
    while (1)
    {
        sem_wait(&queue_occup);
        pthread_mutex_lock(&thread_queue_mutex);
        extract_count_local = get_message(&msg);
        consume_message(&msg);
        sem_post(&queue_left);
        pthread_mutex_unlock(&thread_queue_mutex);

        printf("C %d: сообщение удалено: тип=%X, размер=%X, hash=%X, всего удалено:%zu\n",
               number, msg.type, msg.size, msg.hash, extract_count_local);
        for (int u = 0; u < 10; u++)
        {
            if (cons_status[number - 1] == 0)
                pthread_exit(NULL);
            sleep(1);
        }
    }
}

void consume_message(struct message *message) // удаление сообщения потребителем
{
    uint16_t message_hash = message->hash;
    message->hash = 0;
    uint16_t check_sum = hash(message);
    if (message_hash != check_sum)
    {
        fprintf(stderr, "ошибка проверки контрольных данных: %X != %X\n",
                check_sum, message_hash);
    }
    message->hash = message_hash;
}

///////////////////////////////////////////////////////////////////////

int main()
{
    queue_init(); // инициализация очереди и семафоров
    printf("p - Добавить производителя\n");
    printf("d - Удалить производителя\n");
    printf("c - Добавить потребителя\n");
    printf("x - Удалить потребителя\n");
    printf("l - Вывести очередь на экран\n");
    printf("+ - Увеличить размер очереди\n");
    printf("- - Уменьшить размер очереди\n");
    printf("q - Выход\n");
    char action;
    int num_pros = 0, num_cons = 0;
    pthread_t thread_cons[MAX_LEN], thread_pros[MAX_LEN];
    while (1)
    {
        scanf(" %c", &action);

        if (action == 'p')
        {
            num_pros++;
            pros_status[num_pros - 1] = 1;
            pthread_create(&thread_pros[num_pros - 1], NULL, producer_thread, &num_pros); // создание производителя
        }
        else if (action == 'd')
        {
            if (num_pros > 0)
            {
                num_pros--;
                pthread_cancel(thread_pros[num_pros]);
                pthread_join(thread_pros[num_pros], NULL); // удаление производителя
                printf("Удалён производитель N %d\n", num_pros + 1);
            }
            else
                printf("Нет производителей\n");
        }
        else if (action == 'c')
        {
            num_cons++;
            cons_status[num_cons - 1] = 1;
            pthread_create(&thread_cons[num_cons - 1], NULL, consumer_thread, &num_cons); // создание потребителя
        }
        else if (action == 'x')
        {
            if (num_cons > 0)
            {
                num_cons--;
                pthread_cancel(thread_cons[num_cons]);
                pthread_join(thread_cons[num_cons], NULL); // удаление потребителя
                printf("Удалён потребитель N %d\n", num_cons + 1);
            }
            else
                printf("Нет потребителей\n");
        }
        else if (action == 'l')
        {
            print_queue();
        }
        else if (action == '+')
        {
            queue_len++;
            myqueue->buffer = realloc(myqueue->buffer, queue_len * sizeof(struct message));
            if (myqueue->head < myqueue->tail) {
                myqueue->buffer[queue_len - 1] = myqueue->buffer[myqueue->tail];
                myqueue->tail++;
            };
            sem_post(&queue_left);
            printf("Новый размер очереди: %zu\n", queue_len);
        }
        else if (action == '-')
        {
            if (queue_len > 1)
            {
                if (myqueue->current_num == queue_len)
                {
                    printf("Очередь заполнена! Невозможно уменьшить размер!\n");
                }
                else
                {
                    pthread_mutex_lock(&thread_queue_mutex);
                    if (myqueue->head == queue_len - 1)
                    {
                        myqueue->head = 0;
                    }
                    else if (myqueue->head < myqueue->tail)
                    {
                        myqueue->head++;
                        myqueue->buffer[myqueue->head] = myqueue->buffer[queue_len - 1];
                        if (myqueue->tail == queue_len - 1)
                        {
                            myqueue->tail--;
                        }
                    }
                    queue_len--;
                    myqueue->buffer = realloc(myqueue->buffer, queue_len * sizeof(struct message));
                    sem_wait(&queue_left);
                    printf("Новый размер очереди: %zu\n", queue_len);
                    pthread_mutex_unlock(&thread_queue_mutex);
                }
            }
            else
                printf("Размер очереди не может быть меньше 1\n");
        }
        else if (action == 'q')
        {
            printf("Выход из программы...\n");
            for (int j = 0; j < num_pros; j++)
            {
                pthread_cancel(thread_pros[j]);
                pthread_join(thread_pros[j], NULL); // удаление производителя
            }
            for (int j = 0; j < num_cons; j++)
            {
                pthread_cancel(thread_cons[j]);
                pthread_join(thread_cons[j], NULL); // удаление потребителя
            }
            sem_destroy(&queue_left);
            sem_destroy(&queue_occup);
            free(myqueue->buffer);
            exit(0); // выход
        }
        else
            printf("Неверный ввод\n");

        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 100000000;
        nanosleep(&sleep_time, NULL);
    };
    return 0;
}