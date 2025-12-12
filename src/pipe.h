#ifndef PIPE_H
#define PIPE_H

// Execute a pipeline command string like "cat file.txt | sort | uniq"
// Returns 0 on success, non-zero on error
int execute_pipeline_cmd(const char* cmd);

#endif // PIPE_H
