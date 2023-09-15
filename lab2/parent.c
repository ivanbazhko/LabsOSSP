#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 256

int main(int argc, char **argv, char **envp)
{
    char childpath[80] = "";

    pid_t pid;
    int status;

    extern char **environ;
    int i, j, count = 0;
    char *temp;
    while (environ[count] != NULL)
    {
        count++;
    }

    // Сортировка массива строк
    for (i = 0; i < count - 1; i++)
    {
        for (j = i + 1; j < count; j++)
        {
            if (strcmp(environ[i], environ[j]) > 0)
            {
                temp = environ[i];
                environ[i] = environ[j];
                environ[j] = temp;
            }
        }
    }

    // Вывод отсортированных переменных окружения
    for (i = 0; i < count; i++)
    {
        printf("%s\n", environ[i]);
    }

    char action;
    int childnumber;
    int child_status;
    childnumber = 0;

    while (1)
    {
        scanf(" %c", &action);

        if (action == 'q' || childnumber > 99 || childnumber < 0)
        {
            if (childnumber > 99)
                printf("Достигнуто максимальное количество дочерних процессов!\n");
            return 0;
        };

        char childnumname[4];
        char childname[9];
        strcpy(childname, "CHILD_");
        if(childnumber >= 0 && childnumber <= 99) snprintf(childnumname, 3, "%02d", childnumber);
        strcat(childname, childnumname);

        if (action == '+' || action == '*' || action == '&')
        {
            pid = fork(); // Создание дочернего процесса

            if (pid < 0)
            {
                printf("Ошибка запуска процесса\n");
                exit(1);
            }
            else if (pid == 0)
            {
                if (action == '+')
                {
                    char *acpath = getenv("CHILD_PATH"); // получаем значение переменной среды
                    if (acpath == NULL)
                    {
                        printf("Ошибка: не установлен путь к child!\n");
                    }
                    else
                    {
                        char *childargv[] = {childname, argv[1], "+", (char *)0};
                        int err = execve(acpath, childargv, envp); // Выполнение программы child
                        if(err == -1) printf("Ошибка запуска программы child!\n");
                    };
                };
                if (action == '*')
                {
                    char bcpath[100] = "";
                    for (int i = 0; envp[i] != NULL; i++) // Поиск информации о расположении программы child в массиве параметров среды envp
                    {
                        if (strstr(envp[i], "CHILD_PATH") != NULL)
                        {
                            strcpy(bcpath, envp[i]);
                            break;
                        };
                    };
                    if (strcmp(bcpath, "") == 0)
                    {
                        printf("Ошибка: не установлен путь к child!\n");
                    }
                    else
                    {
                        char *childargv[] = {childname, argv[1], "*", (char *)0};
                        char *str1 = strchr(bcpath, '=');
                        char *ptrstr = str1 + 1;
                        int err = execve(ptrstr, childargv, envp); // Выполнение программы child
                        if(err == -1) printf("Ошибка запуска программы child!\n");
                    };
                }
                if (action == '&')
                {
                    char ccpath[100] = "";
                    for (i = 0; i < count; i++)  // Поиск информации о расположении программы child в массиве параметров среды environ
                    {
                        if (strstr(environ[i], "CHILD_PATH") != NULL)
                        {
                            strcpy(ccpath, environ[i]);
                            break;
                        };
                    }
                    if (strcmp(ccpath, "") == 0)
                    {
                        printf("Ошибка: не установлен путь к child!\n");
                    }
                    else
                    {
                        char *childargv[] = {childname, argv[1], "&", (char *)0};
                        char *str1 = strchr(ccpath, '=');
                        char *ptrstr = str1 + 1;
                        int err = execve(ptrstr, childargv, envp); // Выполнение программы child
                        if(err == -1) printf("Ошибка запуска программы child!\n");
                    };
                };
                exit(0);
            };
            wait(&child_status);
            childnumber++;
        };
    };
    return 0;
}