#define _POSIX_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define MAX_PROC 100

pid_t child_pids[MAX_PROC]; // массив для хранения pid дочерних процессов
int child_prints[MAX_PROC];
static int process_count = 0, process_number = 0, print_enabled = 1, force_print = 0, muted_print = 0, terminate_flag = 0;
void print_child_info(int process_count, int cou1, int cou2, int cou3, int cou4);
struct pair
{
    int num1;
    int num2;
};

void print_help()
{
    printf("Commands:\n");
    printf("  +: Create a new child process.\n");
    printf("  -: Kill the last child process.\n");
    printf("  l: List all child processes.\n");
    printf("  k: Kill all child processes.\n");
    printf("  s: Enable printing for child process.\n");
    printf("  g: Disable printing for child process.\n");
    printf("  p: Ask information from child process.\n");
    printf("  h: Print help message.\n");
    printf("  q: Finish and quit.\n");
}

void handle_signal(int receivedSignal)
{
    if (receivedSignal == SIGTERM)
    {
        terminate_flag = 1;
    }
    else if (receivedSignal == SIGUSR1)
    {
        print_enabled = 1;
    }
    else if (receivedSignal == SIGUSR2)
    {
        print_enabled = 0;
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &handle_signal;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
}

void list_processes()
{
    printf("Parent process with PID %d\n", getpid()); // print parent process PID
    printf("Child processes running:\n");
    for (int i = 1; i <= process_count; i++)
    {
        printf("C_%02d with PID %d\n", i, child_pids[i]); // print process name and PID
    }
}

void kill_processes()
{
    printf("Terminating all child processes\n");
    for (int i = 1; i <= process_count; i++)
    {
        kill(child_pids[i], SIGTERM); // send terminate signal to each child process
        wait(&child_pids[i]);
    }
    process_count = 0;
}

void kill_last_process()
{
    if (process_count > 0)
    {
        kill(child_pids[process_count], SIGTERM); // send terminate signal to last child process
        wait(&child_pids[process_count]);
        process_count--;
        printf("Terminated last child process\n");
        printf("%d processes left\n", process_count);
    }
}

int child_function() // функция дочернего процесса
{
    printf("Child process C_%02d created with PID %d\n", process_count, getpid());
    child_pids[process_count] = getpid(); // add child PID to array
    struct pair curPair;
    curPair.num1 = 0;
    curPair.num2 = 0;
    // Счётчики количества пар
    int cou1 = 0;
    int cou2 = 0;
    int cou3 = 0;
    int cou4 = 0;
    struct timespec delay = {0, 100000000};
    while (1)
    {
        for (int u = 0; u < 101; u++)
        {
            if (terminate_flag)
            {
                exit(1);
            }
            nanosleep(&delay, NULL);
            if (curPair.num1 == 0 && curPair.num2 == 0)
            {
                curPair.num1 = 0;
                curPair.num2 = 1;
                cou2++;
                continue;
            }
            else if (curPair.num1 == 0 && curPair.num2 == 1)
            {
                curPair.num1 = 1;
                curPair.num2 = 0;
                cou3++;
                continue;
            }
            else if (curPair.num1 == 1 && curPair.num2 == 0)
            {
                curPair.num1 = 1;
                curPair.num2 = 1;
                cou4++;
                continue;
            }
            else if (curPair.num1 == 1 && curPair.num2 == 1)
            {
                curPair.num1 = 0;
                curPair.num2 = 0;
                cou1++;
                continue;
            }
        };
        if (print_enabled == 1)
        {
            print_child_info(process_count, cou1, cou2, cou3, cou4);
        };
    }
    return 0;
}

void print_child_info(int process_count, int cou1, int cou2, int cou3, int cou4) // вывод статистики
{
    fputc('C', stdout);
    fputc('_', stdout);
    printf("%02d", process_count);
    fputc(':', stdout);
    fputc('P', stdout);
    fputc('I', stdout);
    fputc('D', stdout);
    fputc(':', stdout);
    fputc(' ', stdout);
    printf("%d", getpid());
    fputc('\n', stdout);

    fputc('C', stdout);
    fputc('_', stdout);
    printf("%02d", process_count);
    fputc(':', stdout);
    fputc('P', stdout);
    fputc('P', stdout);
    fputc('I', stdout);
    fputc('D', stdout);
    fputc(':', stdout);
    fputc(' ', stdout);
    printf("%d", getppid());
    fputc('\n', stdout);

    fputc('C', stdout);
    fputc('_', stdout);
    printf("%02d", process_count);
    fputc(':', stdout);
    fputc('{', stdout);
    fputc('0', stdout);
    fputc(',', stdout);
    fputc('0', stdout);
    fputc('}', stdout);
    fputc(':', stdout);
    fputc(' ', stdout);
    printf("%d", cou1);
    fputc('\n', stdout);

    fputc('C', stdout);
    fputc('_', stdout);
    printf("%02d", process_count);
    fputc(':', stdout);
    fputc('{', stdout);
    fputc('0', stdout);
    fputc(',', stdout);
    fputc('1', stdout);
    fputc('}', stdout);
    fputc(':', stdout);
    fputc(' ', stdout);
    printf("%d", cou2);
    fputc('\n', stdout);

    fputc('C', stdout);
    fputc('_', stdout);
    printf("%02d", process_count);
    fputc(':', stdout);
    fputc('{', stdout);
    fputc('1', stdout);
    fputc(',', stdout);
    fputc('0', stdout);
    fputc('}', stdout);
    fputc(':', stdout);
    fputc(' ', stdout);
    printf("%d", cou3);
    fputc('\n', stdout);

    fputc('C', stdout);
    fputc('_', stdout);
    printf("%02d", process_count);
    fputc(':', stdout);
    fputc('{', stdout);
    fputc('1', stdout);
    fputc(',', stdout);
    fputc('1', stdout);
    fputc('}', stdout);
    fputc(':', stdout);
    fputc(' ', stdout);
    printf("%d", cou4);
    fputc('\n', stdout);
}

int main()
{
    signal(SIGTERM, handle_signal);
    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);
    print_help();
    pid_t child_pid;
    char input[80];
    while (1)
    {
        if (fgets(input, sizeof(input), stdin))
        {
            if (input[0] == '+')
            {
                process_count++;
                if (process_count >= MAX_PROC)
                {
                    printf("Достигнуто максимальное число процессов\n");
                    process_count--;
                }
                else
                {
                    child_pid = fork();
                    if (child_pid == 0)
                    {
                        child_function();
                    }
                    else if (child_pid > 0)
                    {
                        child_pids[process_count] = child_pid; // add child PID to array
                        child_prints[process_count] = 1;
                    }
                    else
                    {
                        perror("fork");
                        exit(1);
                    }
                };
            }
            else if (input[0] == 'l') // вывод всех процессов
            {
                list_processes();
            }
            else if (input[0] == 'k') // удаление всех дочерних процессов
            {
                kill_processes();
            }
            else if (input[0] == '-') // удаление последнего процесса
            {
                kill_last_process();
            }
            else if (input[0] == 'q') // удаление всех процессов и выход из программы
            {
                printf("Exiting\n");
                kill_processes();
                exit(0);
            }
            else if (input[0] == 'h') // вывод сообщения о функциях программы
            {
                print_help();
            }
            else if (input[0] == 'g') // запрет выводить статистику
            {
                if (input[1] == '\0' || input[1] == '\n')
                {
                    printf("Disabling printing for all child processes\n");
                    for (int i = 1; i <= process_count; i++)
                    {
                        kill(child_pids[i], SIGUSR2);
                        printf("C_%02d: printing disabled\n", i);
                        child_prints[i] = 0;
                    }
                }
                else
                {
                    int process_num = atoi(&input[2]);
                    if (process_num > 0 && process_num <= process_count)
                    {
                        printf("Disabling printing for child process C_%02d\n", process_num);
                        kill(child_pids[process_num], SIGUSR2);
                        printf("C_%02d: printing disabled\n", process_num);
                        child_prints[process_num] = 0;
                    }
                }
            }
            else if (input[0] == 's') // разрешение выводить статистику
            {
                if (input[1] == '\0' || input[1] == '\n')
                {
                    printf("Enabling printing for all child processes\n");
                    for (int i = 1; i <= process_count; i++)
                    {
                        kill(child_pids[i], SIGUSR1);
                        printf("C_%02d: printing enabled\n", i);
                        child_prints[i] = 1;
                    }
                }
                else
                {
                    int process_num = atoi(&input[2]);
                    if (process_num > 0 && process_num <= process_count)
                    {
                        printf("Enabling printing for child process C_%02d\n", process_num);
                        kill(child_pids[process_num], SIGUSR1);
                        printf("C_%02d: printing enabled\n", process_num);
                        child_prints[process_num] = 1;
                    }
                }
            }
            else if (input[0] == 'p')
            {
                struct timespec sddelay = {15, 0};
                if (input[1] != '\0' && input[1] != '\n')
                {
                    int process_num = atoi(&input[2]);
                    for (int i = 1; i <= process_count; i++)
                    {
                        if (i != process_num) {
                            kill(child_pids[i], SIGUSR2);
                            printf("C_%02d: printing disabled\n", i);
                        }
                        else {
                            kill(child_pids[process_num], SIGUSR1);
                            printf("C_%02d: printing enabled\n", i);
                        };
                    }
                    sleep(10);
                    for (int i = 1; i <= process_count; i++)
                    {
                        kill(child_pids[i], SIGUSR2);
                    }
                    sleep(5);
                    for (int i = 1; i <= process_count; i++)
                    {
                        if (child_prints[i] == 0) {
                            kill(child_pids[i], SIGUSR2);
                            printf("C_%02d: printing disabled\n", i);
                        }
                        else
                        {
                            kill(child_pids[i], SIGUSR1);
                            printf("C_%02d: printing enabled\n", i);
                        }
                    }
                }
            }
            else
            {
                printf("Invalid input\n");
            }
        }
    }
    return 0;
}