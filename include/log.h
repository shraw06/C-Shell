#ifndef LOG_H
#define LOG_H

#include "tokens.h"
#include "validate.h"

void load_from_file(char* history_path, FILE* f_hist, char history_buffer[][MAX_INPUT], int* history_count, int* history_size);

bool should_log(char* input, command* cmd_groups, int commands, char history_buffer[][MAX_INPUT], int history_count);

void add_log(FILE* f_hist, char* history_path, char* input, int* history_size, int* history_count, char history_buffer[][MAX_INPUT]);

void print_log(char* history_path, FILE* f_hist, int* history_size, int* history_count, char history_buffer[][MAX_INPUT]);

void purge_log(char* history_path, FILE* f_hist, int* history_size, int* history_count, char history_buffer[][MAX_INPUT]);



#endif