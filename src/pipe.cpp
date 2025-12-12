#include "pipe.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>

static void execute_cmd(char* cmd) {
    int i = 0;
    int arg_cnt = 0;

    while (cmd[i] == ' ') i++;
    cmd += i;  // Skip all leading spaces

    i = 0;

    while (cmd[i] != '\0') {
        if (cmd[i] == ' ') arg_cnt++;
        i++;
    }
    
    char** argv = (char**)calloc(arg_cnt + 2, sizeof(char*));
    char* argument = NULL;
    i = 0;

    while ((argument = strsep(&cmd, " ")) != NULL) {
        if (strlen(argument) != 0) {
            argv[i] = (char*)calloc(strlen(argument) + 1, sizeof(char));
            strncpy(argv[i], argument, strlen(argument));
            i++;
        }
    }

    argv[i] = NULL;

    if (execvp(argv[0], argv) != 0) {
        fprintf(stderr, "%s: command not found\n", argv[0]);
        _exit(127);
    }
}

int execute_pipeline_cmd(const char* input) {
    if (input == NULL || strlen(input) == 0) return -1;
    
    // Make a mutable copy since we modify it
    char* cmd = strdup(input);
    if (cmd == NULL) return -1;
    
    int* fd = (int*)calloc(2, sizeof(int));
    if (pipe(fd) != 0) {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        free(cmd);
        return errno;
    }

    int prev_cmd_length = 0;
    int i = 0;
    
    while (cmd[i] != '\0') {
        if (cmd[i] == '|') {
            int pid = fork();
            if (pid == -1) {
                fprintf(stderr, "Error forking: %s\n", strerror(errno));
                free(cmd);
                return errno;
            } else if (pid == 0) {
                // Child: redirect stdout to pipe write end
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                char* cur_cmd = (char*)calloc(i + 1 - prev_cmd_length, sizeof(char));
                strncpy(cur_cmd, cmd + prev_cmd_length, i - prev_cmd_length);
                execute_cmd(cur_cmd);
                _exit(127);  // Only reached if exec fails
            } else {
                // Parent: redirect stdin from pipe read end, create new pipe for next stage
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);

                fd = (int*)calloc(2, sizeof(int));
                pipe(fd);
            }
            prev_cmd_length = i + 1;
        }
        i++;
    }
    
    // Execute the last command (after the last pipe)
    char* cur_cmd = (char*)calloc(i + 1 - prev_cmd_length, sizeof(char));
    strncpy(cur_cmd, cmd + prev_cmd_length, i - prev_cmd_length);
    execute_cmd(cur_cmd);
    
    // Only reached if exec fails
    free(cmd);
    return 127;
}

// Standalone main for testing (compile with -DPIPE_STANDALONE)
#ifdef PIPE_STANDALONE
int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: pipe <commands to execute>\n");
        return -1;
    }
    return execute_pipeline_cmd(argv[1]);
}
#endif
