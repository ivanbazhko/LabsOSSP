#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096
#define MAX_NAME 255

int main()
{
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    int received_bytes;
    char *current_dir = calloc(MAX_NAME, sizeof(char));
    int change = 0;

    // создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("Ошибка создания сокета!");
        return 1;
    }

    // настройка адреса сервера
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(8080);

    // подключение к серверу
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Ошибка подключения!");
        return 1;
    }

    // отправка контрольного PING сообщения серверу
    send(client_socket, "PING", strlen("PING"), 0);
    sleep(1);

    // получение текущего каталога
    while ((received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)))
    {
        buffer[received_bytes] = '\0';
        int i = 0;
        while (buffer[i] != '\n')
        {
            printf("%c", buffer[i]);
            i++;
        };
        i++;
        for (int j = 0; buffer[i]; i++, j++)
        {
            current_dir[j] = buffer[i];
        }
        break;
    }

    while (1)
    {
        do
        {
            printf("\n%s>", current_dir);
            fgets(buffer, BUFFER_SIZE, stdin);
        } while (strcmp(buffer, "\n") == 0);

        buffer[strcspn(buffer, "\n")] = '\0'; // Удаление символа новой строки

        if (strstr(buffer, "@"))
        {
            char *file_name = calloc(MAX_NAME, 1);
            int i = 0;
            int j = 1;
            while (buffer[j])
            {
                file_name[i] = buffer[j];
                i++;
                j++;
            }
            file_name[i] = '\0';
            FILE *file = fopen(file_name, "r");
            if (file == NULL)
            {
                puts("Файл не найден!");
                continue;
            }
            while (fgets(buffer, sizeof(buffer), file))
            {
                printf("\n%s>%s\n", current_dir, buffer);

                if (strstr(buffer, "\n"))
                {
                    int i = 0;
                    while (buffer[i] != '\n')
                        i++;
                    buffer[i] = '\0';
                }

                send(client_socket, buffer, strlen(buffer), 0);

                if (strstr(buffer, "CD"))
                {
                    change = 1;
                }

                received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

                buffer[received_bytes] = '\0';

                if (change)
                {
                    int i = 0;
                    int j = 0;
                    while (buffer[j])
                    {
                        current_dir[i] = buffer[j];
                        i++;
                        j++;
                    }
                    current_dir[i] = '\0';
                    change = 0;
                }
                else
                {
                    printf("%s", buffer);
                }

                sleep(1);
            }
            fclose(file);
            continue;
        }

        // отправка запроса на сервер
        send(client_socket, buffer, strlen(buffer), 0);

        if (strstr(buffer, "CD"))
        {
            change = 1;
        }

        // получение ответа от сервера
        received_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (received_bytes <= 0)
        {
            perror("Подключение прервано!");
            break;
        }

        buffer[received_bytes] = '\0';

        if (change)
        {
            int i = 0;
            int j = 0;
            while (buffer[j])
            {
                current_dir[i] = buffer[j];
                i++;
                j++;
            }
            current_dir[i] = '\0';
            change = 0;
        }
        else
        {
            printf("%s", buffer);
        }

        // Проверка условия завершения сеанса
        if (strcmp(buffer, "QUIT") == 0)
        {
            break;
        }
    }

    close(client_socket);
    free(current_dir);
    return 0;
}