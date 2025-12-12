#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>

void execute_cmd(char* cmd) {
    int i = 0;
    int arg_cnt = 0;

    while (cmd[i] == ' ') i++;
    cmd += i;  // Skip all leading spaces, not just 1

    i = 0;

    while (cmd[i] != '\0') {
        if (cmd[i] == ' ') arg_cnt++;
        i++;
    }
    
    char** argv = calloc(arg_cnt+2, sizeof(char*));
    char* argument = NULL;
    i = 0;

    while ((argument  = strsep(&cmd, " ")) != NULL) {
        if (strlen(argument) != 0) {
            argv[i] = calloc(strlen(argument) + 1, sizeof(char));
            strncpy(argv[i], argument, strlen(argument));
            i++;  // Only increment when we actually add an argument
        }
    }

    argv[i] = NULL;

    if (execvp(argv[0], argv) != 0){
        fprintf(stderr, "Error executing command: %s\n", strerror(errno));
        exit(1);
    }
}

int main (int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: pipe <commands to execute>\n");
        exit(-1);
    }

    int* fd = calloc(2, sizeof(int));
    if (pipe(fd) != 0) {
        printf("Error creating pipe: %s\n", strerror(errno));
        exit(errno);
    }

    const char* cmd = argv[1];
    int prev_cmd_length = 0;
    int i = 0;
    
    while (cmd[i] != '\0') {
        if (cmd[i] == '|') {
            int pid = fork();
            if (pid == -1) {
                printf("Error forking: %s\n", strerror(errno));
                exit(errno);
            } else if (pid == 0) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                char* cur_cmd = calloc(i + 1 - prev_cmd_length, sizeof(char));
                strncpy(cur_cmd, cmd + prev_cmd_length, i - prev_cmd_length);
                execute_cmd(cur_cmd);
            } else {
                // Parent: redirect stdin from pipe read end, create new pipe for next stage
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);

                fd = calloc(2, sizeof(int));
                pipe(fd);
            }
            prev_cmd_length = i + 1;
        }
        i++;
    }
     
    // Execute the last command (after the last pipe)
    char* cur_cmd = calloc(i + 1 - prev_cmd_length, sizeof(char));
    strncpy(cur_cmd, cmd + prev_cmd_length, i - prev_cmd_length);
    execute_cmd(cur_cmd);

    return 0;
}

    
