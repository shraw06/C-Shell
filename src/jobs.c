#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include "tokens.h"
#include "jobs.h"

//////////////// LLM Generated Code Begins ////////////////
bool command_has_redirection(command *cmd) {
    for (int i = 0; i < cmd->pipe_count; i++) {
        for (int j = 0; j < cmd->pipes[i].at_commands; j++) {
            token_type type = cmd->pipes[i].atomic_commands[j].type;
            if (type == INPUT || type == OUTPUT || type == APPEND) {
                return true;
            }
        }
    }
    return false;
}


void kill_all_jobs() {
    for (int i = 0; i < job_count; i++) {
        if (active_jobs[i].pid > 0) {
            kill(active_jobs[i].pid, SIGKILL);
        }
    }
}

int get_next_job_number() {
    if (job_count == 0) {
        return 1;
    }
    int max_job = 0;
    for (int i = 0; i < job_count; i++) {
        if (active_jobs[i].job_id > max_job) {
            max_job = active_jobs[i].job_id;
        }
    }
    return max_job + 1;
}
const char* get_process_state(pid_t pid, int* terminated) {
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (result == -1) {
        *terminated = 1;
        return "Terminated";
    }
    if (result == 0) {
        *terminated = 0;
        return "Running";
    }
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        *terminated = 1;
        return "Terminated";
    }
    if (WIFSTOPPED(status)) {
        *terminated = 0;
        return "Stopped";
    }
    return "Running"; 
}

int cmp_jobs(const void *a, const void *b) {
    const background_job *ja = (const background_job *)a;
    const background_job *jb = (const background_job *)b;
    const char *sa = (ja->command_name[0]!='\0') ? ja->command_name : "";
    const char *sb = (jb->command_name[0]!='\0') ? jb->command_name : "";
    return strcmp(sa, sb);
}

void child_run_activities_from_pipe(int read_fd) {
    background_job received_jobs[MAX_JOBS];
        // background_job *received_jobs = malloc(sizeof(background_job) * job_count);

    int count = 0;
    ssize_t bytes_read;

    // Read job structs from the pipe until the parent closes it
    while ((bytes_read = read(read_fd, &received_jobs[count], sizeof(background_job))) > 0) {
        if (count < MAX_JOBS) {
            count++;
        }
    }

    if (count > 0) {
        qsort(received_jobs, count, sizeof(background_job), cmp_jobs);
        for (int i = 0; i < count; i++) {
            printf("[%d] : %s - %s\n", received_jobs[i].pid, received_jobs[i].command_name, received_jobs[i].state);
        }
    }
    fflush(stdout);
}

void print_activities() {
    if (job_count == 0) {
        return;
    }

    // background_job copy[MAX_JOBS];
    background_job *copy = malloc(sizeof(background_job) * job_count);
    if (!copy) {
        // If malloc fails, we can't do anything.
        perror("malloc in print_activities");
        return;
    }
    int n = job_count;
    for (int i = 0; i < n; i++) {
        copy[i].pid = active_jobs[i].pid;
        copy[i].job_id = active_jobs[i].job_id;
        strncpy(copy[i].command_name, active_jobs[i].command_name, sizeof(copy[i].command_name) - 1);
        copy[i].command_name[sizeof(copy[i].command_name)-1] = '\0';
        strncpy(copy[i].state, active_jobs[i].state, sizeof(copy[i].state) - 1);
        copy[i].state[sizeof(copy[i].state)-1] = '\0';
    }

    if (n > 1) qsort(copy, n, sizeof(background_job), cmp_jobs);

    for (int i = 0; i < n; i++) {
        printf("[%d] : %s - %s\n", copy[i].pid, copy[i].command_name, copy[i].state);
    }
    fflush(stdout);
}

// void refresh_jobs() {
//     int write_idx = 0;
//     for (int i = 0; i < job_count; i++) {
//         pid_t pid = active_jobs[i].pid;
//         int status = 0;

//         /* Check non-blocking for exit/stop/continue events */
//         pid_t r = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

//         if (r == 0) {
//             /* no status change; keep entry as-is */
//             if (write_idx != i) active_jobs[write_idx] = active_jobs[i];
//             write_idx++;
//             continue;
//         }

//         if (r == -1) {
//             /* error (e.g. ESRCH) — treat as gone/terminated: drop entry */
//             continue;
//         }

//         /* r > 0: status changed for this pid */
//         if (WIFEXITED(status) || WIFSIGNALED(status)) {
//             /* process terminated -> remove (do not copy) */
//             continue;
//         }

//         if (WIFSTOPPED(status)) {
//             /* mark stopped and keep */
//             background_job tmp = active_jobs[i];
//             strncpy(tmp.state, "Stopped", sizeof(tmp.state) - 1);
//             tmp.state[sizeof(tmp.state) - 1] = '\0';
//             active_jobs[write_idx++] = tmp;
//             continue;
//         }

//         if (WIFCONTINUED(status)) {
//             /* mark running and keep */
//             background_job tmp = active_jobs[i];
//             strncpy(tmp.state, "Running", sizeof(tmp.state) - 1);
//             tmp.state[sizeof(tmp.state) - 1] = '\0';
//             active_jobs[write_idx++] = tmp;
//             continue;
//         }

//         /* fallback: keep entry */
//         if (write_idx != i) active_jobs[write_idx] = active_jobs[i];
//         write_idx++;
//     }

//     /* compact array to only alive/stopped jobs */
//     job_count = write_idx;
// }


void refresh_jobs() {
    int write_idx = 0;
    for (int i = 0; i < job_count; i++) {
        pid_t pid = active_jobs[i].pid;
        int status = 0;

        /* Non-blocking check for status changes */
        pid_t r = waitpid(pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        if (r == 0) {
            /* no status change; keep entry */
            if (write_idx != i) active_jobs[write_idx] = active_jobs[i];
            write_idx++;
            continue;
        }

        if (r == -1) {
            /* error querying this pid (e.g. ESRCH) - treat as terminated abnormally */
            printf("%s with pid %d exited abnormally\n", active_jobs[i].command_name, (int)pid);
            continue; /* drop entry */
        }

        /* r > 0: status changed */
        if (WIFEXITED(status)) {
            /* exited normally */
            printf("%s with pid %d exited normally\n", active_jobs[i].command_name, (int)pid);
            continue; /* drop entry */
        }

        if (WIFSIGNALED(status)) {
            /* terminated by signal -> abnormal */
            printf("%s with pid %d exited abnormally\n", active_jobs[i].command_name, (int)pid);
            continue; /* drop entry */
        }

        if (WIFSTOPPED(status)) {
            /* mark stopped and keep */
            background_job tmp = active_jobs[i];
            strncpy(tmp.state, "Stopped", sizeof(tmp.state) - 1);
            tmp.state[sizeof(tmp.state) - 1] = '\0';
            active_jobs[write_idx++] = tmp;
            continue;
        }

        if (WIFCONTINUED(status)) {
            /* mark running and keep */
            background_job tmp = active_jobs[i];
            strncpy(tmp.state, "Running", sizeof(tmp.state) - 1);
            tmp.state[sizeof(tmp.state) - 1] = '\0';
            active_jobs[write_idx++] = tmp;
            continue;
        }

        /* fallback: keep entry */
        if (write_idx != i) active_jobs[write_idx] = active_jobs[i];
        write_idx++;
    }

    job_count = write_idx;
}

int find_job_index_by_id(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (active_jobs[i].job_id == job_id) return i;
    }
    return -1;
}

int find_most_recent_job_index(void) {
    if (job_count == 0) return -1;
    int idx = 0;
    int max_id = active_jobs[0].job_id;
    for (int i = 1; i < job_count; i++) {
        if (active_jobs[i].job_id > max_id) {
            max_id = active_jobs[i].job_id;
            idx = i;
        }
    }
    return idx;
}

void remove_job_index(int idx) {
    if (idx < 0 || idx >= job_count) return;
    for (int i = idx; i < job_count - 1; i++) {
        active_jobs[i] = active_jobs[i + 1];
    }
    job_count--;
}

//////////////// LLM Generated Code Ends ////////////////////
