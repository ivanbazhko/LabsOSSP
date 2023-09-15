#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>

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
#define MSG_MAX 20

#define OPEN_FLAGS (O_RDWR | O_CREAT | O_TRUNC)
#define MODE (S_IRUSR | S_IWUSR)

#define SHM_OBJECT "/queue"
#define MUTEX "mutex"
#define FREE_SPACE "free_space"
#define ITEMS "items"

sem_t *queue_mutex;
sem_t *queue_left;
sem_t *queue_occup;

pid_t prod_array[MAX_LEN];
size_t prod_number;

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
    struct message buffer[MSG_MAX];
};

struct queue *queue;
static pid_t parent;

///////////////////////////////////////////////////////////////////////

uint16_t hash(struct message *msg);
void queue_init();
size_t put_message(struct message *message);
size_t get_message(struct message *message);
void initialize();
void clear_proc();
void create_producer();
void delete_producer();
void generate_message(struct message *message);
void create_consumer();
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
    queue->add_num = 0;
    queue->del_num = 0;

    queue->current_num = 0;

    queue->head = 0;
    queue->tail = 0;
    memset(queue->buffer, 0, sizeof(queue->buffer));
}

size_t put_message(struct message *message) // добавление сообщения в очередь
{
    if (queue->current_num == MSG_MAX)
    {
        fputs("Переполнение очереди\n", stderr);
        exit(EXIT_FAILURE);
    }

    if (queue->head == MSG_MAX)
    {
        queue->head = 0;
    }

    queue->buffer[queue->head] = *message;
    ++queue->head;
    ++queue->current_num;

    return ++queue->add_num;
}

size_t get_message(struct message *message) // удаление сообщения из очереди
{
    if (queue->current_num == 0)
    {
        fputs("Недостаточно сообщений\n", stderr);
        exit(EXIT_FAILURE);
    }

    if (queue->tail == MSG_MAX)
    {
        queue->tail = 0;
    }

    *message = queue->buffer[queue->tail];
    ++queue->tail;
    --queue->current_num;

    return ++queue->del_num;
}

///////////////////////////////////////////////////////////////////////

void initialize() // инициализация очереди и семафоров
{
    parent = getpid();

    atexit(clear_proc); // удаление процессов при завершении
    int fd = shm_open(SHM_OBJECT, OPEN_FLAGS, MODE);
    if (fd < 0)
    {
        perror("shm error");
        exit(errno);
    }
    if (ftruncate(fd, sizeof(struct queue)))
    {
        perror("ftruncate error");
        exit(errno);
    }
    void *ptr = mmap(NULL, sizeof(struct queue), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        exit(errno);
    }
    queue = (struct queue *)ptr;
    if (close(fd))
    {
        perror("close error");
        exit(errno);
    }
    queue_init();
    queue_mutex = sem_open(MUTEX, OPEN_FLAGS, MODE, 1);
    queue_left = sem_open(FREE_SPACE, OPEN_FLAGS, MODE, MSG_MAX);
    queue_occup = sem_open(ITEMS, OPEN_FLAGS, MODE, 0);
}

void clear_proc() // удаление процессов при завершении
{
    if (getpid() == parent)
    {
        for (size_t i = 0; i < prod_number; ++i)
        {
            kill(prod_array[i], SIGKILL);
            wait(NULL);
        }
        for (size_t i = 0; i < cons_number; ++i)
        {
            kill(cons_array[i], SIGKILL);
            wait(NULL);
        }
    }
    else if (getppid() == parent)
    {
        kill(getppid(), SIGKILL);
    }

    if (shm_unlink(SHM_OBJECT))
    {
        perror("shm_unlink error");
        abort();
    }
    if (sem_unlink(MUTEX) ||
        sem_unlink(FREE_SPACE) ||
        sem_unlink(ITEMS))
    {
        perror("sem_unlink error");
        abort();
    }
}

///////////////////////////////////////////////////////////////////////

void create_producer() // создание производителя
{
    if (prod_number == MAX_LEN - 1)
    {
        fputs("Достигнуто максимальное число производителей\n", stderr);
        return;
    }
    pid_t new_prod_id = prod_array[prod_number] = fork();
    if (new_prod_id < 0)
    {
        perror("fork");
        exit(errno);
    }
    else if (new_prod_id > 0)
    {
        prod_number++;
        return;
    }
    else
    {
        printf("Создан производитель с pid = %d\n", getpid());
        srand(getpid());
        struct message msg;
        size_t add_count_local;
        while (1)
        {
            generate_message(&msg);

            sem_wait(queue_left);

            sem_wait(queue_mutex);
            add_count_local = put_message(&msg);
            sem_post(queue_mutex);

            sem_post(queue_occup);

            printf("%d: сообщение создано: тип=%X, размер=%X, hash=%X, всего создано:%zu\n",
                   getpid(), msg.type, msg.size, msg.hash, add_count_local);

            sleep(10);
        }
    }
}

void delete_producer() // удаление производителя
{
    if (prod_number == 0)
    {
        fputs("Нет производителей!\n", stderr);
        return;
    }

    prod_number--;
    kill(prod_array[prod_number], SIGKILL);
    wait(NULL);
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

void create_consumer() // создание потребителя
{
    if (cons_number == MAX_LEN - 1)
    {
        fputs("Достигнуто максимальное число потребителей\n", stderr);
        return;
    }
    pid_t new_cons_id = cons_array[cons_number] = fork();
    if (new_cons_id < 0)
    {
        perror("fork error");
        exit(errno);
    }
    else if (new_cons_id > 0)
    {
        cons_number++;
        return;
    }
    else
    {
        printf("Создан потребитель с pid = %d\n", getpid());
        struct message msg;
        size_t extract_count_local;
        while (1)
        {
            sem_wait(queue_occup);

            sem_wait(queue_mutex);
            extract_count_local = get_message(&msg);
            sem_post(queue_mutex);

            sem_post(queue_left);

            consume_message(&msg);

            printf("%d: сообщение удалено: тип=%X, размер=%X, hash=%X, всего удалено:%zu\n",
                   getpid(), msg.type, msg.size, msg.hash, extract_count_local);

            sleep(10);
        }
    }
}

void delete_consumer() // удаление потребителя
{
    if (cons_number == 0)
    {
        fputs("Нет потребителей!\n", stderr);
        return;
    }

    cons_number--;
    kill(cons_array[cons_number], SIGKILL);
    wait(NULL);
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
    initialize(); // инициализация очереди и семафоров
    printf("p - Добавить производителя\n");
    printf("d - Удалить производителя\n");
    printf("c - Добавить потребителя\n");
    printf("x - Удалить потребителя\n");
    printf("q - Выход\n");
    char action;
    while (1)
    {
        scanf(" %c", &action);

        if (action == 'p')
        {
            create_producer(); // создание производителя
        }
        else

            if (action == 'd')
        {
            delete_producer(); // удаление производителя
        }
        else

            if (action == 'c')
        {
            create_consumer(); // создание потребителя
        }
        else if (action == 'x')
        {
            delete_consumer(); // удаление потребителя
        }
        else

            if (action == 'q')
        {
            exit(0); // выход
        }
        else

            printf("Неверный ввод\n");
    };
}