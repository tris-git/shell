#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

char *get_word(char *end) {
    *end = 0;
    char ch, *word = NULL;
    int size = 0;
    do {
        ch = getchar();
        word = realloc(word, size + 1);
        if (ch == '\n' || ch == '\t' || ch == ' ') {
            word[size] = '\0';
            *end = ch;
        }
        else 
            word[size] = ch;
        size++;
    } while (!(*end));
    return word;
}

char **get_list() {
    char **list = NULL, end;
    int size, cnt = 0;
    do {
        size = (cnt + 1) * sizeof(char *);
        list = realloc(list, size);
        list[cnt] = get_word(&end);
        cnt++;
    } while (end != '\n');
    list = realloc(list, (cnt + 1) * sizeof(char *));
    list[cnt] = NULL;
    return list;
}

void free_list(char **list) {
    for (int i = 0; list[i] != NULL; i++) 
        free(list[i]);
    free(list);
}

int main(int argc, char **argv) {
    char **list = get_list();
    int fd;
    pid_t pid;
    while (strcmp(list[0], "quit") && strcmp(list[0], "exit")) { 
        if ((pid = fork()) > 0) {
            wait(NULL);
        } 
        else {
            int i = 0, check = -1;
            for(; list[i] != NULL; i++) {
                if (!strcmp(list[i], ">")) {
                    check = 1;
                    break;
                } else if (!strcmp(list[i], "<")) {
                    check = 0;
                    break;
                }
            }
            if (check == -1) {

                if (execvp(list[0], list) < 0) {
                    perror("exec failed");
                    return 1;
                }

            } else if (check == 1) {
                fd = open(list[i+1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                for (int j = i; list[j] != NULL; j++)
                    free(list[j]);
                list[i] = NULL;
                if (pid == 0) {
                    dup2(fd, 1);
                    if (execvp(list[0], list) < 0) {
                        perror("exec failed");
                        return 1;
                    }
                }
                close(fd);
            } else {
                fd = open(list[i+1], O_RDONLY);
                for (int j = i; list[j] != NULL; j++)
                    free(list[j]);
                list[i] = NULL;
                if (pid == 0) {
                    dup2(fd, 0);
                    if (execvp(list[0], list) < 0) {
                        perror("exec failed");
                        return 1;
                    }
                }
            }
        }
        free_list(list);
        list = get_list();
    }

    free_list(list);
    return 0;
}
