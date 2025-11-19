#ifndef REVEAL_H
#define REVEAL_H

#include "tokens.h"
#include <dirent.h>

int ascii_sort(const struct dirent **a, const struct dirent **b);
void list_order(char* path, int a, int l);
void reveal_process(token* at_tk_list, int tk_size, char* home, char* prev);

#endif