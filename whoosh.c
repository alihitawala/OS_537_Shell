//
// Created by alihitawala on 2/5/16.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>

#define BUFFERSIZE 128

void error_output() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *trim_whitespace(char *str) {
    char *last;
    while (isspace(*str)) str++;
    if (*str == 0) return str;
    last = str + strlen(str) - 1;
    while (last > str && isspace(*last)) last--;
    *(last + 1) = 0;
    return str;
}

char **tokenize_string(char *cmd, int *count) {
    char **res = NULL;
    char *p = strtok(cmd, " ");
    int n_spaces = 0;
    while (p) {
        res = realloc(res, sizeof(char *) * ++n_spaces);
        res[n_spaces - 1] = strdup(p);
        p = strtok(NULL, " ");
    }
    res = realloc(res, sizeof(char *) * (n_spaces + 1));
    res[n_spaces] = 0;
    *count = n_spaces;
    return res;
}

int check_if_built_in_command(char **cmds, int length) {
    return length > 0 && ((strcmp(cmds[0], "exit") == 0 && length == 1) || (strcmp(cmds[0], "pwd") == 0 && length == 1)
                          || (strcmp(cmds[0], "cd") == 0 && length < 3) || (strcmp(cmds[0], "path") == 0));
}

int command_exists_in_path(char *command, char **paths, int length, char **actualPath) {
    int i = 1;
    for (; i < length; i++) {
        char *tempPath = strcat(strdup(paths[i]), command);
        struct stat fileStat;
        if (stat(tempPath, &fileStat) >= 0) {
            *actualPath = strdup(tempPath);
            return 0;
        }
    }
    return 1;
}

int run_shell_commands(char **command, int length, char **path, int path_length, char *filename) {
    char *actualPath;
    if (command_exists_in_path(command[0], path, path_length, &actualPath) == 0) {
        int out = 0, err = 0, save_out = 0, save_err = 0;
        if (filename != NULL) {
            out = open(strcat(strdup(filename), ".out"), O_RDWR | O_CREAT | O_TRUNC, 0600);
            if (-1 == out) { return 1; }
            err = open(strcat(strdup(filename), ".err"), O_RDWR | O_CREAT | O_TRUNC, 0600);
            if (-1 == err) { return 1; }
            save_out = dup(fileno(stdout));
            save_err = dup(fileno(stderr));
            if (-1 == dup2(out, fileno(stdout))) { return 1; }
            if (-1 == dup2(err, fileno(stderr))) { return 1; }
        }
        int child_stat;
        pid_t child_pid;
        child_pid = fork();
        if (child_pid == 0) {
            execv(actualPath, command);
            return 1;
        }
        else {
            waitpid(child_pid, &child_stat, 0);
            if (filename != NULL) {
                fflush(stdout);close(out);
                fflush(stderr);close(err);
                dup2(save_out, fileno(stdout));close(save_out);
                dup2(save_err, fileno(stderr));close(save_err);
            }
        }
        return 0;
    }
    return 1;
}

int check_command_syntax(char **commands, int length, char **filename) {
    int i = 0;
    for (; i < length; i++) {
        if (strcmp(commands[i], ">") == 0) {
            if (i != length - 2 || i == 0) return 1;
            commands[i] = 0;
            *filename = commands[i + 1];
        }
    }
    return 0;
}

int main() {
    char buffer[BUFFERSIZE];
    char **path;
    int path_length = 0;
    while (1) {
        char *filename = 0;
        printf("%s", "whoosh > ");
        if (fgets(buffer, BUFFERSIZE, stdin) != NULL) {
            int command_length = 0;
            char **commands = tokenize_string(trim_whitespace(buffer), &command_length);
            if (check_if_built_in_command(commands, command_length) != 0) {
                if (strcmp(commands[0], "exit") == 0)
                    exit(0);
                else if (strcmp(commands[0], "pwd") == 0) {
                    char buff[PATH_MAX + 1];
                    printf("%s\n", getcwd(buff, PATH_MAX + 1));
                }
                else if (strcmp(commands[0], "cd") == 0) {
                    char *dir = command_length == 1 ? getenv("HOME") : commands[1];
                    if (chdir(dir) != 0)
                        error_output();
                }
                else if (strcmp(commands[0], "path") == 0) {
                    path = commands;
                    path_length = command_length;
                }
            }
            else if (command_length != 0 && (check_command_syntax(commands, command_length, &filename) != 0 ||
                     run_shell_commands(commands, command_length, path, path_length, filename) != 0))
                error_output();
        }
        else
            error_output();
    }
}