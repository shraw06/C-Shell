#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tokens.h"
#include "log.h"

void load_from_file(char* history_path, FILE* f_hist, char history_buffer[][MAX_INPUT], int* history_count, int* history_size)
{
    FILE* f = fopen(history_path, "r");
    if(!f)
    {
        *history_count = 0;
        *history_size = 0;
        // perror("fopen");
        return;
    }
    int i = 0;
    fflush(f);
    while(i<15 && fgets(history_buffer[i], MAX_INPUT, f))
    {
        history_buffer[i][strcspn(history_buffer[i], "\n")] = '\0';
        i += 1;
    }

    *history_count = i;
    *history_size = i;
    fclose(f);
   return;  
}

bool should_log(char* input, command* cmd_groups, int commands, char history_buffer[][MAX_INPUT], int history_count)
{
    if (history_count > 0) {
        int last_index = (history_count - 1) % 15;
        if (strcmp(input, history_buffer[last_index]) == 0) return false;
    }

    for(int i=0; i<commands; i++)
    {
        for(int j=0; j<cmd_groups[i].pipe_count; j++)
        {
            //  if (cmd_groups[i].pipes[j].at_commands > 0 &&
            //     cmd_groups[i].pipes[j].atomic_commands[0].value != NULL &&
            //     strcmp("log", cmd_groups[i].pipes[j].atomic_commands[0].value) == 0)
            //     return false;

             if (cmd_groups[i].pipes[j].at_commands > 0) {
                token *first = &cmd_groups[i].pipes[j].atomic_commands[0];
                if (first->type == NAME && first->value[0] != '\0' && strcmp("log", first->value) == 0) {
                    return false;
                }
            }
            // if(history_count>0 && strcmp(input, history_buffer[history_count-1])==0)
            // return false;

            // if(strcmp("log", cmd_groups[i].pipes[j].atomic_commands[0].value)==0)
            // return false;
        }
    }
    return true;
}

void add_log(FILE* f_hist, char *history_path, char *input, int *history_size, int *history_count, char history_buffer[][MAX_INPUT])
{
    // Write into circular buffer at position = history_count % 15 (newest position)
    int pos = (*history_count) % 15;
    strncpy(history_buffer[pos], input, MAX_INPUT - 1);
    history_buffer[pos][MAX_INPUT - 1] = '\0';

    (*history_count)++; // running total
    if (*history_size < 15) (*history_size)++;

    // Persist entire buffer oldest -> newest
    int start;
    if (*history_count <= 15) {
        start = 0;
    } else {
        start = (*history_count) % 15; // oldest entry index in buffer
    }

    FILE *f = fopen(history_path, "w");
    if (!f) {
        perror("fopen");
        return;
    }
    for (int i = 0; i < *history_size; i++) {
        int idx = (start + i) % 15;
        fprintf(f, "%s\n", history_buffer[idx]);
    }
    fclose(f);
    // if(*history_size<15)
    // strcpy(history_buffer[*history_count], input);
    // else
    // {
    //     for(int i=1; i<15; i++)
    //     strcpy(history_buffer[i-1], history_buffer[i]);

    //     strcpy(history_buffer[14], input);
    // }

    // *history_count = (*(history_count) == 15) ? 15 : *(history_count) + 1;
    // *history_size = (*(history_size) == 15) ? 15 : *(history_size) + 1;

    

    // f_hist = freopen(history_path, "w", f_hist);
    // if (!f_hist) {
    //     perror("freopen");
    //     return;
    // }
    
    
    //     for(int i=0; i < *history_size; i++)
    //     fprintf(f_hist, "%s\n", history_buffer[i]);
    

    // fflush(f_hist);

}

void print_log(char* histpry_path, FILE* f_hist, int* history_size, int* history_count, char history_buffer[][MAX_INPUT])
{
    if (*history_size == 0) return;

    int start;
    if (*history_count <= 15) {
        start = 0;
    } else {
        start = (*history_count) % 15;
    }

    for (int i = 0; i < *history_size; i++) {
        int idx = (start + i) % 15;
        printf("%s\n", history_buffer[idx]);
    }
    return;
    //     load_from_file(histpry_path, f_hist, history_buffer, history_count, history_size);
    
    //     for(int i=0; i<*history_size; i++)
    //     printf("%s\n", history_buffer[i]);

    // return;
     
}

void purge_log(char* history_path, FILE* f_hist, int* history_size, int* history_count, char history_buffer[][MAX_INPUT])
{
    
    for(int i=0; i<*history_size; i++)
    {
        history_buffer[i][0] = '\0';
    }

    *history_size = 0;
    *history_count = 0;

    FILE *f = fopen(history_path, "w");
    if (!f) {
        perror("fopen");
        return;
    }
    fclose(f);
    return;
    // f_hist = freopen(history_path, "w", f_hist);
    // if(!f_hist)
    // {
    //     perror("freopen");
    //     return;
    // }

    // fflush(f_hist);
    
}

