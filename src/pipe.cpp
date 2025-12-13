#include "pipe.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <set>
#include <string>


static std::set<std::string> pipe_builtins = {"echo", "pwd", "type", "byee", "shell_name", "cd", "history"};

static bool is_builtin(const char* cmd) {
    return pipe_builtins.find(std::string(cmd)) != pipe_builtins.end();
}

static void execute_builtin(int argc, char** argv) {
    if (argc == 0 || argv[0] == NULL) _exit(1);
    
    std::string cmd = argv[0];
    
    if (cmd == "echo") {
        for (int i = 1; i < argc; i++) {
            printf("%s", argv[i]);
            if (i + 1 < argc) printf(" ");
        }
        printf("\n");
        _exit(0);
    }
    else if (cmd == "pwd") {
        char buffer[4096];
        if (getcwd(buffer, sizeof(buffer)) != NULL) {
            printf("%s\n", buffer);
            _exit(0);
        }
        _exit(1);
    }
    else if (cmd == "type") {
        if (argc != 2) {
            fprintf(stderr, "Usage: type <command>\n");
            _exit(1);
        }
        std::string target = argv[1];

        if (pipe_builtins.find(target) != pipe_builtins.end()) {
            printf("%s is a shell builtin\n", argv[1]);
            _exit(0);
        }
        
        char* path_env = getenv("PATH");
        if (path_env != NULL) {
            char* path_copy = strdup(path_env);
            char* dir = strtok(path_copy, ":");
            while (dir != NULL) {
                char full_path[4096];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, argv[1]);
                if (access(full_path, X_OK) == 0) {
                    printf("%s is %s\n", argv[1], full_path);
                    free(path_copy);
                    _exit(0);
                }
                dir = strtok(NULL, ":");
            }
            free(path_copy);
        }
        printf("%s: not found\n", argv[1]);
        _exit(1);
    }
    else if (cmd == "shell_name") {
        printf("Taran's Shell running\n");
        _exit(0);
    }
    else if (cmd == "cd") {
        fprintf(stderr, "cd: cannot be used in a pipeline\n");
        _exit(1);
    }
    else if (cmd == "byee") {
        _exit(0);
    }
    
    _exit(1);
}

static void execute_cmd(char* cmd) {
    int i = 0;
    int arg_cnt = 0;

    while (cmd[i] == ' ') i++;
    cmd += i; 

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
    int argc = i;

  
    if (is_builtin(argv[0])) {
        execute_builtin(argc, argv);
        _exit(0);  
    }

    
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
