#define _GNU_SOURSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <time.h>
#include <limits.h>

#define MAX_CLIENTS 4
#define BUFFER_SIZE 4096
#define MAX_NAME 255
#define MAX_LIST_SIZE 255

ssize_t readlink(const char *restrict link_pathname, char *restrict buffer, size_t buffer_size);

// Структура для передачи данных в поток обработки клиента
struct transf_data
{
    int socket;
};

int clients_count = 0;
int is_end = 1;
char *root;
char inform[80];

char *conc_str(char *first_str, char *second_str)
{
    char *result = malloc(MAX_NAME);
    int i = 0;
    int j = 0;
    int k = 0;

    while (first_str[j])
    {
        result[i] = first_str[j];
        i++;
        j++;
    }

    while (second_str[k])
    {
        result[i] = second_str[k];
        i++;
        k++;
    }

    result[i] = '\0';

    return result;
}

// обработка запроса ECHO
char *pr_echo(char buffer[BUFFER_SIZE])
{
    char *foo_buffer = malloc(BUFFER_SIZE * sizeof(char));

    int i = 0;
    int j = sizeof("ECHO");
    while (buffer[i])
    {
        foo_buffer[i++] = buffer[j++];
    }
    foo_buffer[i] = '\0';

    return foo_buffer;
}

// обработка запроса INFO
char *pr_info()
{
    strcpy(inform, "Вас приветсвует учебный сервер 'server'\n");
    return inform;
}

// обработка запроса LIST
char *pr_list(char *current_dir)
{
    int element_count = 0;
    DIR *directory;
    struct dirent *dire;
    struct stat entryInfo;
    char pathName[MAX_NAME];
    char *file_names = (char *)malloc(sizeof(char) * MAX_LIST_SIZE);

    chdir(current_dir);

    directory = opendir(current_dir); // открытие каталога
    if (!directory)
    {
        return file_names;
    }

    strcpy(file_names, "");

    while ((dire = readdir(directory)) != NULL)
    {

        if (!strcmp(".", dire->d_name) || !strcmp("..", dire->d_name))
        {
            continue;
        }

        char name[MAX_NAME];
        element_count++;

        memcpy(name, (char *)dire->d_name, MAX_NAME);

        (void)strncpy(pathName, current_dir, MAX_NAME);
        (void)strncat(pathName, "/", MAX_NAME - 1);
        (void)strncat(pathName, dire->d_name, MAX_NAME);

        if (lstat(pathName, &entryInfo) == 0)
        { // Получение информации о файле
            if (S_ISDIR(entryInfo.st_mode))
            {
                strcat(name, "/");
            }
            else if (S_ISLNK(entryInfo.st_mode))
            {
                char link_path[MAX_NAME];

                ssize_t len = readlink(conc_str("./", name), link_path, sizeof(link_path) - 1);
                link_path[len] = '\0';
                struct stat lkstat;

                lstat(link_path, &lkstat);
                if (S_ISLNK(lkstat.st_mode))
                {
                    strcat(name, "-->>");
                }
                else
                {
                    strcat(name, "-->");
                }

                strcat(name, link_path);
            };
        };

        strcat(name, "\n");

        file_names = (char *)realloc(file_names, sizeof(char) * element_count * MAX_NAME);

        strcat(file_names, name);
    }

    return file_names;
}

// обработка запроса CD
char *pr_cd(char *current_dir, char *buffer)
{

    char *dir_name = calloc(MAX_NAME, sizeof(char));
    int i = 0;
    int j = sizeof("CD");
    while (buffer[i])
    {
        dir_name[i++] = buffer[j++];
    }
    dir_name[i] = '\0';

    char *buff = calloc(MAX_NAME, 1);

    chdir(current_dir);
    chdir(dir_name);
    getcwd(buff, MAX_NAME);

    if (strlen(buff) < strlen(root))
    {
        puts("Попытка выйти за пределы root");
        chdir(current_dir);
        getcwd(buff, MAX_NAME);
    }

    return buff;
}

// обработчик клиента
void *client_func(void *arg)
{
    struct transf_data *data = (struct transf_data *)arg;
    int client_socket = data->socket;

    char *current_dir = calloc(MAX_NAME, sizeof(char));
    char buffer[BUFFER_SIZE];
    int received_bytes;

    getcwd(current_dir, MAX_NAME);

    while ((received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)))
    {
        buffer[received_bytes] = '\0';
        if (strstr("PING", buffer))
        {
            break;
        }
    }
    send(client_socket, pr_info(), strlen(pr_info()), 0);
    send(client_socket, current_dir, strlen(current_dir), 0);

    while ((received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[received_bytes] = '\0';

        // Обработка запроса
        if (strstr(buffer, "ECHO"))
        {
            // Эхо-запрос
            send(client_socket, pr_echo(buffer), strlen(buffer), 0);
        }
        else if (strstr(buffer, "QUIT"))
        {
            // Запрос на завершение сеанса
            clients_count--;
            is_end = 0;
            break;
        }
        else if (strstr(buffer, "INFO"))
        {
            // Запрос на получение информации о сервере
            send(client_socket, pr_info(), strlen(pr_info()), 0);
        }
        else if (strstr(buffer, "CD"))
        {
            // Запрос на изменение текущего каталога
            current_dir = pr_cd(current_dir, buffer);

            strcpy(buffer, current_dir);

            send(client_socket, buffer, strlen(buffer), 0);
        }
        else if (strstr(buffer, "LIST"))
        {
            // Запрос на получение списка файловых объектов
            if (strcmp(pr_list(current_dir), "") != 0)
            {
                send(client_socket, pr_list(current_dir), strlen(pr_list(current_dir)), 0);
            }
            else {
                char errmsg[30];
                strcpy(errmsg, "Нет файлов\n");
                send(client_socket, errmsg, strlen(errmsg), 0);
            };
        }
        else
        {
            // Неизвестный запрос
            char response[] = "Неверный ввод";
            send(client_socket, response, strlen(response), 0);
        }
    }

    close(client_socket);
    free(data);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    pthread_t threads[MAX_CLIENTS];

    // установка корневого каталога
    root = calloc(MAX_NAME, 1);
    if (argv[1])
    {
        strcpy(root, argv[1]);
        chdir(root);
        getcwd(root, MAX_NAME);
    }
    else
    {
        getcwd(root, MAX_NAME);
    }

    // Создание сокета
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Ошибка создания сокета");
        return 1;
    }

    // Настройка адреса
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    // Привязка сокета
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Ошибка привязки");
        return 1;
    }

    // Прослушивание входящих соединений
    if (listen(server_socket, MAX_CLIENTS) == -1)
    {
        perror("Ошибка прослушивания");
        return 1;
    }

    printf("Сервер запущен...\n");

    while (is_end || clients_count)
    {

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        struct timeval pause;
        pause.tv_sec = 1;

        fd_set tmp_fds = read_fds;

        int res = select(server_socket + 1, &read_fds, NULL, NULL, &pause);
        if (res == -1)
        {
            perror("Ошибка!");
            exit(1);
        }
        else if (res == 0)
        {
            sleep(1);
            continue;
        }

        if (!FD_ISSET(server_socket, &tmp_fds))
        {
            continue;
        }

        // принятие соединения
        socklen_t client_address_len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1)
        {
            perror("Ошибка принятия соединения!");
            continue;
        }

        // Создание нового потока для обработки клиента
        if (clients_count < MAX_CLIENTS)
        {
            struct transf_data *data = (struct transf_data *)malloc(sizeof(struct transf_data));
            data->socket = client_socket;

            if (pthread_create(&threads[clients_count], NULL, client_func, (void *)data) != 0)
            {
                perror("Ошибка создания потока!");
                close(client_socket);
                free(data);
            }
            else
            {
                clients_count++;
                printf("Клиент подключён. Всего клиентов: %d\n", clients_count);
            }
        }
        else
        {
            printf("Достигнуто максимальное количество клиентов!\n");
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}