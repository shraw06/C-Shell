#ifndef JOBS_H
#define JOBS_H

#include <stdbool.h>
#include "tokens.h"

bool command_has_redirection(command *cmd);

void kill_all_jobs();

int get_next_job_number();

const char* get_process_state(pid_t pid, int* terminated);

int cmp_jobs(const void *a, const void *b);

void child_run_activities_from_pipe(int read_fd);

void print_activities();

void refresh_jobs();

int find_job_index_by_id(int job_id);

int find_most_recent_job_index(void);

void remove_job_index(int idx);

#endif