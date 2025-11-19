#ifndef TOKENS_H
#define TOKENS_H

#include <limits.h>
#include <sys/types.h>

typedef struct {
    pid_t pid;                           
    int job_id;                          
    char command_name[MAX_INPUT];    
    char state[16];    
} background_job;

#define MAX_JOBS 4194303

extern background_job active_jobs[MAX_JOBS];
extern int job_count;

typedef enum {
    NAME,
    AND,
    SEMICOLON,
    PIPE,
    INPUT,
    OUTPUT,
    APPEND
} token_type;

typedef struct {
    token_type type;
    char value[4097];
}token;

typedef struct pipeline {
    token* atomic_commands; //an individual atomic command
    int at_commands; //number of tokens in the atomic command
}pipeline;

typedef struct command {
    pipeline pipes[MAX_INPUT];
    int pipe_count;
    int is_amp;
}command;

void tokenise(char* input, token* token_list, int* tk_size);

#endif