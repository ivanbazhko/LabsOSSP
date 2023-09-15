#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[], char *envp[])
{
    if (argc < 2) {
        printf("Недостаточно аргуметнов командной строки\n");
        return 0;
    };
    printf("My name is %s!\n", argv[0]); // Вывод имени, pid, ppid
    printf("My pid is %d!\n", getpid());
    printf("My ppid is %d!\n", getppid());
    extern char **environ;
    char *fileName = argv[1];
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char helpingString[256] = "";
    fp = fopen(fileName, "r"); // Открытие файла environ
    if (fp == NULL)
    {
        printf("\nОшибка открытия файла\n");
        return 0;
    }
    if (strcmp(argv[2], "+") == 0) // Вывод переменных окружения
    {
        while ((read = getline(&line, &len, fp)) != -1)
        {
            for (int g = 0; line[g] != '\0'; g++)
                if (line[g] == '\n')
                    line[g] = '\0';
            char *ln = getenv(line);
            if (ln == NULL)
                printf("%s is undefined\n", line);
            else
                printf("%s=%s\n", line, ln);
        }
    }
    if (strcmp(argv[2], "*") == 0) // Вывод переменных окружения
    {
        while ((read = getline(&line, &len, fp)) != -1)
        {
            for (int g = 0; line[g] != '\0'; g++)
                if (line[g] == '\n')
                    line[g] = '\0';
            for (int i = 0; envp[i] != NULL; i++) // Поиск информации в массиве параметров среды envp
            {
                strcpy(helpingString, "");
                if (strncmp(line, envp[i], strlen(line)) == 0 && envp[i][strlen(line)] == '=')
                {
                    strcpy(helpingString, envp[i]);
                    break;
                };
            };
            if (strcmp(helpingString, "") == 0)
            {
                printf("%s is undefined\n", line);
            }
            else
            {
                printf("%s\n", helpingString);
            };
        }
    }
    if (strcmp(argv[2], "&") == 0) // Вывод переменных окружения
    {
        while ((read = getline(&line, &len, fp)) != -1)
        {
            for (int g = 0; line[g] != '\0'; g++)
                if (line[g] == '\n')
                    line[g] = '\0';
            strcpy(helpingString, "");
            for (int i = 0; environ[i] != NULL; i++) // Поиск информации о расположении программы child в массиве параметров среды environ
            {
                strcpy(helpingString, "");
                if (strncmp(line, environ[i], strlen(line)) == 0 && environ[i][strlen(line)] == '=')
                {
                    strcpy(helpingString, environ[i]);
                    break;
                }
            }
            if (strcmp(helpingString, "") == 0)
            {
                printf("%s is undefined\n", line);
            }
            else
            {
                printf("%s\n", helpingString);
            };
        }
    }
    fclose(fp);
    return 0;
}