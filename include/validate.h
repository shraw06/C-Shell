#ifndef VALIDATE_H
#define VALIDATE_H

#include "tokens.h"

int validate_names(char* name);
int parse_atomic(token* token_list, int tk_size);
int parse_command_groups(token* token_list, int tk_size, command* cmd_groups, int* commands);
int parse_shell_command(token* token_list, int tk_size, command* cmd_groups, int* commands);

#endif