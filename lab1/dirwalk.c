#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

char **addinf(char **array, char *newLine, int number) { //Добавление названия файла в массив
	array = realloc(array, (number + 1) * sizeof(char*));
	*(array + number) = calloc(PATH_MAX + 1, sizeof(char));
	strcpy(*(array + number), newLine);
	return array;
}

void dirwalk(char ***array, char *eDir, int fl, int optD, int optF, int optL, int *amount) { //Обход каталогов
    DIR *dir = opendir(eDir);
    if( dir == NULL ) { //Проверка, открылся ли файл
    	return;
    }
    struct dirent *entry = readdir(dir);
    char pathName[PATH_MAX + 1];
	while(entry != NULL) {
    	struct stat entryInfo;
    	if((strncmp(entry->d_name, ".", PATH_MAX) == 0) || (strncmp(entry->d_name, "..", PATH_MAX) == 0)) {
        	entry = readdir(dir);
        	continue;
    	}
	    (void)strncpy(pathName, eDir, PATH_MAX);
        (void)strncat(pathName, "/", PATH_MAX);
        (void)strncat(pathName, entry->d_name, PATH_MAX);
        if(lstat(pathName, &entryInfo) == 0) { //Получение информации о файле
            if(S_ISDIR(entryInfo.st_mode)) {
			    if(optD == 1 || fl == 1) {            
					*array = addinf(*array, pathName, (*amount)++);
			    }
                dirwalk(array, pathName, fl, optD, optF, optL, amount);
            } 
		    else if(S_ISREG(entryInfo.st_mode)) { 
			    if(optF == 1 || fl == 1) {            
					*array = addinf(*array, pathName, (*amount)++);
			    }
            } 
		    else if(S_ISLNK(entryInfo.st_mode)) { 
                char targetName[PATH_MAX + 1];
                if(readlink(pathName, targetName, PATH_MAX) != -1) {
				    if(optL == 1 || fl == 1) {            
						*array = addinf(*array, pathName, (*amount)++);
				    }
                } 
			}
        }    
	    else {
            printf("Ошибка %s: %s\n", pathName, strerror(errno));
        }
 	    entry = readdir(dir);
	}
	(void)closedir(dir);
}

char **sortNames(char **arr, int n) { //Сортировка методом пузырёк массива строк
    int i, j;
    char temp[PATH_MAX + 1];
    for (i = 0; i < n - 1; i++) {
        for (j = 0; j < n - i - 1; j++) {
            if (strcmp(*(arr+j), *(arr + j + 1)) > 0) {
                strcpy(temp, *(arr+j));
                strcpy(*(arr+j), *(arr + j + 1));
                strcpy(*(arr + j + 1), temp);
            }
        }
    }
	return arr;
}

int main(int argc, char **argv) {
	char option[PATH_MAX];
	char direct[PATH_MAX];
	int fl = 1;
	int amount = 0;
    int optD = 0, optF = 0, optL = 0, optS = 0;
	strcpy(direct,".");
	for(int u = 1; u < argc; u++) { //Получение аргументов коммандной строки
		if(strstr(argv[u], "-f") != NULL || strstr(argv[u], "-d") != NULL || strstr(argv[u], "-l") != NULL|| strstr(argv[u], "-s") != NULL) { 
			if(strstr(argv[u], "f") != NULL) {
				optF = 1;
				fl = 0;
			};
			if(strstr(argv[u], "l") != NULL) {
				optL = 1;
				fl = 0;
			};
			if(strstr(argv[u], "d") != NULL) {
				optD = 1;
				fl = 0;
			};
			if(strstr(argv[u], "s") != NULL) optS = 1;
		} else {
			strcpy(direct, argv[u]);
		}
	};
	char **array = calloc(1, sizeof(char*));
    dirwalk(&array, direct, fl, optD, optF, optL, &amount); //Обход каталогов
	if(optS) array = sortNames(array, amount); //Сортировка результата
	for(int i = 0; i < amount; i++) { //Вывод результата
		printf("%s\n", *(array + i));
	};
    return 0;
}