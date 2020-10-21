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

char **get_list(char *end_ptr) {
    char **list = NULL, end;
    int size, cnt = 0;
    do {
        size = (cnt + 1) * sizeof(char *);
        list = realloc(list, size);
        list[cnt] = get_word(&end);
        *end_ptr = end;
        if (!strcmp(list[cnt], "|")) {
            free(list[cnt]);
            break;
        }
        cnt++;
    } while (end != '\n');
    list = realloc(list, (cnt + 1) * sizeof(char *));
    list[cnt] = NULL;
    return list;
}

char ***get_cmd(int *pipes) {
    char ***big_list = NULL, end;
    int size, cnt = 0;
    *pipes = 0;
    do {
        size = (cnt + 1) * sizeof(char **);
        big_list = realloc(big_list, size);
        big_list[cnt] = get_list(&end);
        cnt++;
    } while (end != '\n');
    big_list = realloc(big_list, (cnt + 1) * sizeof(char **));
    big_list[cnt] = NULL;
    for (; big_list[*pipes] != NULL; (*pipes)++);
    return big_list;
}

void free_list(char ***list) {
    for (int i = 0; list[i] != NULL; i++) {
        for (int j = 0; list[i][j] != NULL; j++)
            free(list[i][j]);
        free(list[i]);
    }
    free(list);
}

int redirect(char **cmd) {
    int i;
    for (i = 0; cmd[i] != NULL; i++)
        if (!strcmp(cmd[i], ">") || !strcmp(cmd[i], "<"))
            break;
    if (cmd[i] == NULL)
        return -1;
    int fd;
    int stream = -1;
    if (!strcmp(cmd[i], ">")) {
        fd = open(cmd[i+1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        stream = 1;
    }
    else if (!strcmp(cmd[i], "<")) {
        stream = 0;
        fd = open(cmd[i+1], O_RDONLY);
    }
    if (stream > -1) {
        dup2(fd, stream);
    }
    for (int j = i; cmd[j] != NULL; j++)
        free(cmd[j]);
    cmd[i] = NULL;
    return fd;
}

void exec_cmd(char **cmd) {
    int fd = redirect(cmd);
    if (execvp(cmd[0], cmd) < 0) {
        perror("exec failed");
        exit(1);
    }
    if (fd > -1)
        close(fd);
}

//
void exec_with_pipes(char ***list, int pipes) {
    pid_t pid;
    int fd = -1;
    int (*pipefd)[2] = malloc(sizeof(int[2]) * (pipes - 1));
            for (int i = 0; i < pipes; i++) {
                if (i != pipes - 1)
                    pipe(pipefd[i]);

                if ((pid = fork()) == 0) {
                    if (i == 0 || i == pipes - 1)
                        fd = redirect(list[i]);
                    if (i != 0)
                        dup2(pipefd[i - 1][0], 0);
                    if (i != pipes - 1)
                        dup2(pipefd[i][1], 1);
                    pipes--;
                    for (int j = 0; j <= i; j++) {
                        if (j == pipes)
                            break;
                        close(pipefd[j][0]);
                        close(pipefd[j][1]);
                    }
                    if (execvp(list[i][0], list[i]) < 0) {
                        free_list(list);
                        perror("exec failed");
                        close(fd);
                        exit(1);
                    }
                }
            }
            for (int i = 0; i < pipes; i++) {
                if (i != pipes - 1) {
                    close(pipefd[i][0]);
                    close(pipefd[i][1]);
                }
                wait(NULL);
            }
            if (fd != 0 && fd != 1)
                close(fd);
            free(pipefd);
}
//

int main(int argc, char **argv) {
    int pipes;
    char ***list = get_cmd(&pipes);
    pid_t pid;
    while (strcmp(list[0][0], "quit") && strcmp(list[0][0], "exit")) {
        if (pipes > 1) {
            exec_with_pipes(list, pipes);
        }
        else {
            if ((pid = fork()) > 0) {
                wait(NULL);
            }
            else {
                exec_cmd(list[0]);
                return 0;
            }
        }
        free_list(list);
        list = get_cmd(&pipes);
    }
    free_list(list);
    return 0;
}
