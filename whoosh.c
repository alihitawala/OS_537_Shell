//
// Created by alihitawala on 2/5/16.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <linux/limits.h>

#define BUFFERSIZE 128

void errorOutput() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *trimWhitespace(char str[]) {
    char *end;
    while (isspace(*str)) str++;
    if (*str == 0)
        return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = 0;
    return str;
}

char **tokenizeString(char *cmd, int *count) {
    char **res = NULL;
    char *p = strtok(cmd, " ");
    int n_spaces = 0;
    while (p) {
        res = realloc(res, sizeof(char *) * ++n_spaces);
        if (res == NULL) {
            errorOutput();
        }
        res[n_spaces - 1] = strdup(p);
        p = strtok(NULL, " ");
    }
    res = realloc(res, sizeof(char *) * (n_spaces + 1));
    res[n_spaces] = 0;
    *count = n_spaces;
    return res;
}

int checkIfBuiltInCommand(char **cmds, int length) {
    return length > 0 && ((strcmp(cmds[0], "exit") == 0 && length == 1) || (strcmp(cmds[0], "pwd") == 0 && length == 1)
                          || (strcmp(cmds[0], "cd") == 0 && length < 3) ||
                          (strcmp(cmds[0], "path") == 0 && length > 1));
}

int commandExistsInPath(char* command, char** paths, int length, char** actualPath) {
    int i=1;
    for (;i<length;i++) {
        char* tempPath = strcat(strdup(paths[i]), command);
        struct stat fileStat;
        if(stat(tempPath, &fileStat) >= 0) {
            *actualPath = strdup(tempPath);
            return 0;
        }
    }
    return 1;
}

int runShellCommands(char** command, int length, char** path, int path_length) {
    char* actualPath;
    if (commandExistsInPath(command[0], path, path_length, &actualPath) == 0) {
        int child_stat;
        pid_t child_pid;
        child_pid = fork();
        if (child_pid == 0) {
            execv(actualPath, command);
            return 1;
        }
        else
            waitpid(child_pid, &child_stat, 0);
        return 0;
    }
    return 1;
}

char** save_path(char** cmds, int length) {
    char **result = malloc(sizeof(char**) * length);
    int i=1;
    for(;i<length;i++) {
        result[i-1] = strdup(cmds[i]);
    }
    return result;
}

int main() {
    char buffer[BUFFERSIZE];
    char **path;
    int path_length = 0;
    while (1) {
        printf("%s", "whoosh > ");
        if (fgets(buffer, BUFFERSIZE, stdin) != NULL) {
            int command_length = 0;
            char **cmds = tokenizeString(trimWhitespace(buffer), &command_length);
            if (checkIfBuiltInCommand(cmds, command_length) != 0) {
                if (strcmp(cmds[0], "exit") == 0)
                    exit(0);
                else if (strcmp(cmds[0], "pwd") == 0) {
                    char *cwd;
                    char buff[PATH_MAX + 1];
                    cwd = getcwd(buff, PATH_MAX + 1);
                    printf("%s\n", cwd);
                }
                else if (strcmp(cmds[0], "cd") == 0) {
                    char *dir = command_length == 1 ? getenv("HOME") : cmds[1];
                    if (chdir(dir) != 0)
                        errorOutput();
                }
                else if (strcmp(cmds[0], "path") == 0) {
                    path = cmds;//save_path(cmds, command_length);
                    path_length = command_length;
                }
            }
            else if (runShellCommands(cmds, command_length, path, path_length) != 0)
                    errorOutput();
        }
        else {
            errorOutput();
        }
    }
}