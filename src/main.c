#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>

#include "prompt.h"
#include "tokens.h"
#include "validate.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "jobs.h"

//////////////LLM Generated Code Begins////////////////
volatile pid_t current_fg_pgid = 0;
pid_t shell_pgid;



static void sigint_handler(int sig) {
    (void)sig;
    if (current_fg_pgid > 0) {
        /* send SIGINT to the foreground process group */
        kill(-current_fg_pgid, SIGINT);
    }
}

static void sigtstp_handler(int sig) {
    (void)sig;
    if (current_fg_pgid > 0) {
        /* send SIGTSTP to the foreground process group */
        kill(-current_fg_pgid, SIGTSTP);
    }
}

int command_has_redirection_or_pipe(command* cmd_group) {
    if (cmd_group->pipe_count > 1) {
        return 1;
    }
    for (int i = 0; i < cmd_group->pipes[0].at_commands; ++i) {
        token_type type = cmd_group->pipes[0].atomic_commands[i].type;
        if (type == INPUT || type == OUTPUT || type == APPEND) {
            return 1;
        }
    }
    return 0;
}

int check_redirections(command* cmd_group) {
    for (int j = 0; j < cmd_group->pipe_count; j++) {
        token* at_tk_list = cmd_group->pipes[j].atomic_commands;
        int tk_size = cmd_group->pipes[j].at_commands;

        for (int k = 0; k < tk_size; k++) {
            if (at_tk_list[k].type == INPUT) {
                k++; // Move to filename
                if (k >= tk_size) break;
                int fd = open(at_tk_list[k].value, O_RDONLY);
                if (fd < 0) {
                    printf("No such file or directory\n");
                    return -1; // Error
                }
                close(fd); // Close immediately, we're just checking
            } else if (at_tk_list[k].type == OUTPUT) {
                k++; // Move to filename
                if (k >= tk_size) break;
                int fd = open(at_tk_list[k].value, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    printf("Unable to create file for writing\n");
                    return -1; // Error
                }
                close(fd);
            } else if (at_tk_list[k].type == APPEND) {
                k++; // Move to filename
                if (k >= tk_size) break;
                int fd = open(at_tk_list[k].value, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (fd < 0) {
                    printf("Unable to create file for writing\n");
                    return -1; // Error
                }
                close(fd);
            }
        }
    }
    return 0; // Success
}
////////////////////LLM Generated Code Ends////////////////////
void execute_command(char* history_path, FILE* f_hist, char* input, command* cmd_groups, int commands, int tk_size, char* home, char* prev, int* history_count, int* history_size, char history_buffer[][MAX_INPUT], int is_subshell)
{
    for (int i = 0; i < commands; i++)
        {
            if (is_subshell) {
                cmd_groups[i].is_amp = 0;
            }

            pid_t representative_pid = 0;
            int pipe_num = cmd_groups[i].pipe_count;
            
            pid_t *child_pids = malloc(sizeof(pid_t) * pipe_num);
            int child_count = 0;
            if (!child_pids) {
                perror("malloc");
                exit(1);
            }

            if (check_redirections(&cmd_groups[i]) != 0) {
                free(child_pids);
                continue; 
            }
            for (int pj = 0; pj < pipe_num; pj++) {
               if (cmd_groups[i].pipes[pj].at_commands <= 0) continue;
               token *first = &cmd_groups[i].pipes[pj].atomic_commands[0];
               if (first->type != NAME || first->value[0] == '\0') continue;
               if (strcmp(first->value, "log") != 0) continue;

               int tk_count = cmd_groups[i].pipes[pj].at_commands;
               token *ats = cmd_groups[i].pipes[pj].atomic_commands;

               /* log purge -> must run in parent */
               if (tk_count >= 2 && ats[1].type == NAME && strcmp(ats[1].value, "purge") == 0) {
                   purge_log(history_path, f_hist, history_size, history_count, history_buffer);
                   /* prevent child from running anything for this pipe element */
                   cmd_groups[i].pipes[pj].at_commands = 0;
                   continue;
               }

               /* log execute N -> run the fetched command in parent (do not add to history) */
                // if (tk_count >= 3 && ats[1].type == NAME && strcmp(ats[1].value, "execute") == 0 && ats[2].type == NAME) {
                //     int idx = atoi(ats[2].value);
                //     if (idx <= 0 || *history_count - idx < 0) {
                //         fprintf(stderr, "Invalid index\n");
                //         /* mark empty to avoid child execution */
                //         cmd_groups[i].pipes[pj].at_commands = 0;                       continue;
                //     }
                //     int pos = (((*history_count) - idx) % 15 + 15) % 15;
                //     char cmdbuf[MAX_INPUT];
                //     strcpy(cmdbuf, history_buffer[pos]);

                //     /* parse and execute the fetched command in parent without logging it */
                //     token *tks = malloc(MAX_INPUT * sizeof(token));
                //     int tks_size = 0;
                //     tokenise(cmdbuf, tks, &tks_size);

                //     command *cmd_groups_new = malloc(MAX_INPUT * sizeof(command));
                //     int commands_new = 0;
                //     for (int ii = 0; ii < MAX_INPUT; ii++) {
                //         cmd_groups_new[ii].pipe_count = 0;
                //         cmd_groups_new[ii].is_amp = 0;
                //         for (int jj = 0; jj < MAX_INPUT; jj++) cmd_groups_new[ii].pipes[jj].at_commands = 0;
                //     }
                //     if (parse_shell_command(tks, tks_size, cmd_groups_new, &commands_new) == 0 || cmdbuf[0] == '\0') {
                //      fprintf(stderr, "Invalid Syntax!\n");
                //         free(tks);
                //         free(cmd_groups_new);
                //         cmd_groups[i].pipes[pj].at_commands = 0;
                //         continue;
                //     }
                //     free(tks);
                //     free(cmd_groups_new);
                //     cmd_groups[i].pipes[pj].at_commands = 0;
                // }
            }


            if (pipe_num == 1 && cmd_groups[i].is_amp == 0 && !command_has_redirection(&cmd_groups[i])) {
                if (cmd_groups[i].pipes[0].at_commands > 0) {                    
                    token *first = &cmd_groups[i].pipes[0].atomic_commands[0];
                    if (first->type == NAME && first->value[0] != '\0') {
                        if (strcmp(first->value, "hop") == 0) {
                            int tk_size_hop = cmd_groups[i].pipes[0].at_commands;
                            hop_process(cmd_groups[i].pipes[0].atomic_commands, tk_size_hop, home, prev);
                            free(child_pids);
                            continue;
                        } else if (strcmp(first->value, "activities") == 0) {
                            refresh_jobs();
                            print_activities();
                            free(child_pids);
                            continue;
                        } else if (strcmp(first->value, "log") == 0) {
                            /* handle log in parent so purge/execute update parent state */
                            int tk_size_log = cmd_groups[i].pipes[0].at_commands;
                            token *at_tk_list = cmd_groups[i].pipes[0].atomic_commands;

                            if (tk_size_log == 1) {
                                print_log(history_path, f_hist, history_size, history_count, history_buffer);
                                free(child_pids);
                               continue;                            }
                            /* safe checks before accessing at_tk_list[1] */
                            if (tk_size_log >= 2 && at_tk_list[1].type == NAME && at_tk_list[1].value[0] != '\0') {
                                if (strcmp(at_tk_list[1].value, "purge") == 0) {
                                    purge_log(history_path, f_hist, history_size, history_count, history_buffer);                                    free(child_pids);
                                    continue;
                                } 
                                else if (strcmp(at_tk_list[1].value, "execute") == 0) {
                                    if (tk_size_log < 3 || at_tk_list[2].type != NAME) {
                                        fprintf(stderr, "Invalid syntax!\n");
                                        free(child_pids);
                                        continue;                                    }
                                    int idx = atoi(at_tk_list[2].value);
                                    if (idx <= 0 || *history_count - idx < 0) {
                                        fprintf(stderr, "Invalid index\n");
                                        free(child_pids);
                                        continue;
                                    }
                                    /* fetch command string from circular buffer */
                                    int pos = (((*history_count) - idx) % 15 + 15) % 15;
                                    char *cmd = malloc(MAX_INPUT);
                                    if (!cmd) {
                                        perror("malloc");
                                        free(child_pids);
                                        continue;
                                    }
                                    strcpy(cmd, history_buffer[pos]);

                                    /* tokenize, parse and execute the fetched command in parent.
                                       We do NOT add this executed command to history (per requirement). */
                                    token *tks = malloc(MAX_INPUT * sizeof(token));
                                    int tks_size = 0;
                                    tokenise(cmd, tks, &tks_size);

                                    command *cmd_groups_new = malloc(MAX_INPUT * sizeof(command));
                                    int commands_new = 0;
                                    for (int ii = 0; ii < MAX_INPUT; ii++) {
                                        cmd_groups_new[ii].pipe_count = 0;
                                        cmd_groups_new[ii].is_amp = 0;
                                        for (int jj = 0; jj < MAX_INPUT; jj++) {
                                            cmd_groups_new[ii].pipes[jj].at_commands = 0;
                                        }
                                    }

                                    if (parse_shell_command(tks, tks_size, cmd_groups_new, &commands_new) == 0 || cmd[0] == '\0') {
                                        printf("Invalid Syntax!\n");
                                        free(cmd);
                                        free(tks);
                                        free(cmd_groups_new);
                                        free(child_pids);
                                        continue;                                    }
                                    /* Execute the fetched command (this will fork as needed). */
                                    execute_command(history_path, f_hist, cmd, cmd_groups_new, commands_new, tks_size, home, prev, history_count, history_size, history_buffer, 0);

                                    free(cmd);
                                    free(tks);
                                    free(cmd_groups_new);
                                   free(child_pids);
                                   continue;
                               }
                           }
                       }
                       else if (strcmp(first->value, "fg") == 0) {
                        /* fg [job_number] : bring job to foreground */
                        int tk_size_fg = cmd_groups[i].pipes[0].at_commands;
                        token *ats = cmd_groups[i].pipes[0].atomic_commands;
                        int idx = -1;
                        if (tk_size_fg >= 2 && ats[1].type == NAME) {
                            int jid = atoi(ats[1].value);
                            idx = find_job_index_by_id(jid);
                        } else {
                                idx = find_most_recent_job_index();
                        }
                        if (idx == -1) {
                            printf("No such job\n");
                            free(child_pids);
                            continue;
                        }
                        /* print the entire command */
                        pid_t pgid = active_jobs[idx].pid;

                        printf("%s\n", active_jobs[idx].command_name);
                        fflush(stdout);

                        tcsetpgrp(STDIN_FILENO, pgid);
                        /* if stopped, resume it */
                        if (strcmp(active_jobs[idx].state, "Stopped") == 0) {
                            if (kill(-pgid, SIGCONT) == -1) {
                                perror("kill");
                            } else {
                                   strncpy(active_jobs[idx].state, "Running", sizeof(active_jobs[idx].state)-1);
                                active_jobs[idx].state[sizeof(active_jobs[idx].state)-1] = '\0';
                            }
                        }

                        /* bring to foreground: set current fg pgid and wait until done or stopped */
                        current_fg_pgid = pgid;
                        int status;
                        //while (1) {
                            pid_t w = waitpid(-pgid, &status, WUNTRACED);
                            if (w == -1) {
                                //if (errno == ECHILD) break;
                                //break;
                                perror("waitpid (fg)");
                            }
                            if (WIFSTOPPED(status)) {
                                strncpy(active_jobs[idx].state, "Stopped", sizeof(active_jobs[idx].state)-1);
                                active_jobs[idx].state[sizeof(active_jobs[idx].state)-1] = '\0';
                                //printf("[%d] Stopped %s\n", active_jobs[idx].job_id, active_jobs[idx].command_name);
                                //break;
                            }
                            else
                            {
                                remove_job_index(idx);
                            }
                            // if (WIFEXITED(status) || WIFSIGNALED(status)) {
                            //     /* keep reaping until no children in the group remain */
                            //     continue;
                            // }
                        //}
                        /* if job's process group no longer exists, remove job entry */
                        // if (kill(-pgid, 0) == -1 && errno == ESRCH) {
                        //     remove_job_index(idx);
                        // }

                        tcsetpgrp(STDIN_FILENO, shell_pgid);
                        current_fg_pgid = 0;
                        free(child_pids);
                        continue;
                    }
                    else if (strcmp(first->value, "bg") == 0) {
                        /* bg [job_number] : resume stopped job in background */
                        int tk_size_bg = cmd_groups[i].pipes[0].at_commands;
                        token *ats = cmd_groups[i].pipes[0].atomic_commands;
                        int idx = -1;
                        if (tk_size_bg >= 2 && ats[1].type == NAME) {
                            int jid = atoi(ats[1].value);
                            idx = find_job_index_by_id(jid);
                        } else {
                            idx = find_most_recent_job_index();
                        }
                        if (idx == -1) {
                            printf("No such job\n");
                            free(child_pids);
                            continue;
                        }
                        if (strcmp(active_jobs[idx].state, "Stopped") != 0) {
                            printf("Job already running\n");
                            free(child_pids);
                            continue;
                        }
                        pid_t pgid = active_jobs[idx].pid;
                        if (kill(-pgid, SIGCONT) == -1) {
                            perror("kill");
                           free(child_pids);
                            continue;
                        }
                        strncpy(active_jobs[idx].state, "Running", sizeof(active_jobs[idx].state)-1);
                        active_jobs[idx].state[sizeof(active_jobs[idx].state)-1] = '\0';
                        printf("[%d] %s &\n", active_jobs[idx].job_id, active_jobs[idx].command_name);
                        free(child_pids);
                        continue;
                    }
                    }
                }
            }
                
            refresh_jobs();

            int fds[2 * (pipe_num - 1)];
            if(pipe_num > 1)
            {
            for (int f = 0; f < pipe_num - 1; f++)
            {
                if(pipe(&fds[2 * f])<0)
                {
                    perror("pipe error");
                    exit(1);
                }
            }
            }

            //number of atomic commands will be pipe_num
            int fd_in = -1;
            int fd_out = -1;
            for (int j = 0; j < pipe_num; j++)
            {
                //dealing with each atomic command
                int tk_size = cmd_groups[i].pipes[j].at_commands;
                token *at_tk_list = cmd_groups[i].pipes[j].atomic_commands;

                int pid = fork();
                
                if (pid < 0)
                {
                    
                    perror("Fork error");
                    exit(1);
                }
                else if (pid == 0)
                {
                    /////////////////LLM Generated Code Starts////////////////////
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);
                    /////////////////LLM Generated Code Ends////////////////////

                    pid_t child_pid = getpid();
                    if (j == 0) { // First command in a pipeline
                        representative_pid = child_pid;
                        // The child puts itself into a new process group. Its PGID is the same as its PID.
                        setpgid(0, 0);
                    } 

                    // /* child: put in its own process group (initially its pid) */
                    // if (setpgid(0, 0) < 0) {
                    //     /* ignore errors */
                    // }
                    // /* restore default signal handling in child */
                    // signal(SIGINT, SIG_DFL);
                    // signal(SIGTSTP, SIG_DFL);

                    // printf("Child process\n");
                    if (cmd_groups[i].is_amp == 1) {
                    int dev_null_fd = open("/dev/null", O_RDONLY);
                    if (dev_null_fd != -1) {
                        dup2(dev_null_fd, 0); // Redirect stdin to /dev/null
                        close(dev_null_fd);
                    }
                }

                
                    char *argv[MAX_INPUT];
                    int argc = 0;
                    char *input_file = NULL;
                    char *output_file = NULL;
                    int append = 0;

                    if(pipe_num>1)
                    {
                    // handling pipes
                    if (j == 0)
                    {
                        dup2(fds[1], 1); //write into the pipe
                    }
                    else if (j == pipe_num - 1)
                    {
                        dup2(fds[2 * (pipe_num - 1) - 2], 0); //read into the pipe
                    }
                    else
                    {
                        dup2(fds[(j - 1) * 2], 0);
                        dup2(fds[(j * 2) + 1], 1);
                    }

                    for(int f=0; f<2*(pipe_num-1); f++)
                    {
                        close(fds[f]);
                    }
                }

                    // file redirection
                    for (int k = 0; k < tk_size; k++)
                    {
                        if (at_tk_list[k].type == NAME)
                        {
                            argv[argc++] = at_tk_list[k].value;
                        }
                        else if (at_tk_list[k].type == INPUT)
                        {
                            if(input_file) free(input_file);
                            input_file = (char *)malloc(sizeof(char) * MAX_INPUT);
                            strcpy(input_file, at_tk_list[++k].value);
                        }
                        else if (at_tk_list[k].type == OUTPUT)
                        {
                            if(output_file) free(output_file);
                            output_file = (char *)malloc(sizeof(char) * MAX_INPUT);
                            strcpy(output_file, at_tk_list[++k].value);
                        }
                        else if (at_tk_list[k].type == APPEND)
                        {
                            if(output_file) free(output_file);
                            output_file = (char *)malloc(sizeof(char) * MAX_INPUT);
                            strcpy(output_file, at_tk_list[++k].value);
                            append = 1;
                        }
                    }
                 

                    argv[argc] = NULL;
                    if(argc==0)
                    {
                        _exit(0);
                    }

                    if (input_file)
                    {
                         fd_in = open(input_file, O_RDONLY);
                         if(fd_in < 0)
                         {
                            printf("No such file or directory\n");
                            exit(1);
                         }
                        dup2(fd_in, 0);
                        close(fd_in);
                    }

                    if (output_file)
                    {
                        if(!append)
                        {
                             fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                             if(fd_out < 0)
                         {
                            printf("Unable to create file for writing\n");
                            exit(1);
                         }
                            dup2(fd_out, 1);
                            close(fd_out);
                        }
                        else
                        {
                            fd_out = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                            if(fd_out < 0)
                         {
                            printf("Unable to create file for writing\n");
                            exit(1);
                         }
                            dup2(fd_out, 1);
                            close(fd_out);
                        }
                    }

                    if (strcmp(argv[0], "reveal") == 0)
                    {
                         token *r_tokens = malloc(tk_size * sizeof(token));
                        int r_size = 0;
                        for (int k=0; k<tk_size; k++) {
                            if (at_tk_list[k].type == INPUT || at_tk_list[k].type == OUTPUT || at_tk_list[k].type == APPEND)
                            {
                                k++;
                                continue;
                            }
                            r_tokens[r_size++] = at_tk_list[k];
                            // fprintf(stderr, "r_size is %d\n", r_size);
                            // for (int ii=0; ii<r_size; ii++) {
                            //     fprintf(stderr, "r_tokens[%d] = '%s'\n", ii, r_tokens[ii].value);
                            // }
                        }

                        // for(int i=0; i<r_size; i++)
                        // printf("%s\n", r_tokens[i].value);
                        reveal_process(r_tokens, r_size, home, prev);
                        free(r_tokens);
                        fflush(stdout);
                        _exit(0);
                    }

                    else if (strcmp(argv[0], "hop") == 0)
                    {
                        hop_process(at_tk_list, tk_size, home, prev);
                        fflush(stdout);
                        _exit(0);
                    }

                    else if (strcmp(argv[0], "log") == 0)
                    {
                        //log_process(at_tk_list, tk_size, home, prev);
                        //exit(0);

                        if(tk_size==1)
                        {
                            print_log(history_path, f_hist, history_size, history_count, history_buffer);
                        }
                        else if(strcmp(at_tk_list[1].value, "purge")==0)
                        {
                            purge_log(history_path, f_hist, history_size, history_count, history_buffer);
                        }
                        else if(strcmp(at_tk_list[1].value, "execute")==0)
                        {
                            if (tk_size < 3 || at_tk_list[2].type != NAME) {
                                fprintf(stderr, "Invalid syntax!\n");
                                _exit(1);
                            }
                            
                            int idx = atoi(at_tk_list[2].value);

                            if(idx<=0 || *history_count - idx < 0)
                            {
                            fprintf(stderr, "%s\n", "Invalid syntax!");
                            exit(1);
                            }

                            char* cmd = (char*)malloc(MAX_INPUT*sizeof(char));
                            // strcpy(cmd, history_buffer[(*history_count - idx)]);
                            int pos = ((*history_count - idx) % 15 + 15) % 15;
                            strcpy(cmd, history_buffer[pos]);

                            token* tks = (token*)malloc(MAX_INPUT*sizeof(token));
                            int tks_size = 0;
                            tokenise(cmd, tks, &tks_size);
                            // for(int i=0; i<tks_size; i++)
                            // printf("%s\n", tks[i].value);

                            command *cmd_groups_new = (command *)malloc(MAX_INPUT * sizeof(command));
                            int commands_new = 0;
                            for (int i = 0; i < MAX_INPUT; i++)
                            {
                                cmd_groups_new[i].pipe_count = 0;
                                cmd_groups_new[i].is_amp = 0;
                                for (int j = 0; j < MAX_INPUT; j++)
                                {
                                    cmd_groups_new[i].pipes[j].at_commands = 0;
                                }
                            }

                            // if (parse_shell_command(tks, tks_size, cmd_groups_new, &commands_new) == 0 || cmd[0] == ' ' || cmd[0] == '\t' || cmd[0] == '\r' || cmd[0] == '\0')
                            // {
            
                            //     printf("Invalid Syntax!\n");
                            //     exit(0);
                            // }

                            if (parse_shell_command(tks, tks_size, cmd_groups_new, &commands_new) == 0)
                            {
            
                                printf("Invalid Syntax!\n");
                                exit(0);
                            }

                            execute_command(history_path, f_hist, cmd, cmd_groups_new, commands_new, tks_size, home, prev, history_count, history_size, history_buffer, 1);

                            free(cmd);
                            free(tks);
                            _exit(0);
                        }
                        
                        fflush(stdout);
                        _exit(0);                        
                    }

                    else if(strcmp(argv[0], "activities") == 0)
                    {
                        print_activities();
                        _exit(0);
                    }

                    else if (strcmp(argv[0], "ping") == 0)
                    {
                    //////////////////LLM Generated Code Starts////////////////////
                       if (argc < 3) {
                            fprintf(stderr, "Invalid syntax!\n");
                            _exit(1);
                        }
                        char *endptr = NULL;
                        long target = strtol(argv[1], &endptr, 10);
                        if (endptr == argv[1] || *endptr != '\0' || target <= 0) {
                            fprintf(stderr, "Invalid syntax!\n");
                            _exit(1);
                        }
                        endptr = NULL;
                        long sig_in = strtol(argv[2], &endptr, 10);
                        if (endptr == argv[2] || *endptr != '\0') {
                            fprintf(stderr, "Invalid syntax!\n");
                            _exit(1);
                        }
                        int actual_sig = (int)(((sig_in % 32) + 32) % 32); /* normalize to 0..31 */

                        if (kill((pid_t)target, actual_sig) == -1) {
                            if (errno == ESRCH) {
                                fprintf(stderr, "No such process found\n");
                            } else {
                                perror("kill");
                            }
                            _exit(1);
                        }
                        printf("Sent signal %d to process with pid %ld\n", actual_sig, target);
                        _exit(0);
                        /////////////////LLM Generated Code Ends////////////////////
                   }

                    else
                    {
                        execvp(argv[0], argv);
                        fprintf(stderr, "%s\n", "Command not found!");
                        exit(1);
                    }
                }
                 else { // parent
                    // printf("Parent process\n");
                    child_pids[child_count++] = pid;

                    if (j == 0) {
                        representative_pid = pid;
                        // /* make the child the leader of its pgid */
                        // if (setpgid(pid, pid) < 0 && errno != EACCES) {
                        //     /* ignore races where child already setpgid */
                        // }
                        // /* for foreground jobs, remember the pgid to forward signals */
                        // if (cmd_groups[i].is_amp == 0) {
                        //     current_fg_pgid = pid;
                        // }
                    }
                    setpgid(pid, representative_pid);
                    // else {
                    //     /* join the pipeline process group */
                    //     if (setpgid(pid, representative_pid) < 0 && errno != EACCES) {
                    //         /* ignore races */
                    //     }
                    // }
                }
            }

            for(int f=0; f<2*(pipe_num-1); f++)
            close(fds[f]);

            if(fd_in!=-1)
            close(fd_in);

            if(fd_out!=-1)
            close(fd_out);

            if(!is_subshell) {
            if (cmd_groups[i].is_amp!=1)
            {
                tcsetpgrp(STDIN_FILENO, representative_pid);
                current_fg_pgid = representative_pid;
                int stopped_flag = 0;
                for (int k = 0; k < child_count; k++) {
                    int status;
                    pid_t w = waitpid(child_pids[k], &status, WUNTRACED);
                    if (w == -1) {
                        perror("waitpid");
                        //continue;
                    }
                    if (WIFSTOPPED(status)) {
                        stopped_flag = 1;
                    }
                }

                tcsetpgrp(STDIN_FILENO, shell_pgid);
                current_fg_pgid = 0;

                if (stopped_flag && representative_pid != 0) {
                    if (job_count < MAX_JOBS) {
                        // active_job *aj = &active_jobs[job_count];
                        background_job *aj = &active_jobs[job_count];
                        aj->pid = representative_pid;
                        aj->job_id = get_next_job_number();
                        strncpy(aj->state, "Stopped", sizeof(aj->state) - 1);
                        aj->state[sizeof(aj->state) - 1] = '\0';

                        ////////////////LLM Generated Code Begins//////////////
                        /* build command_name safely (same logic as background add) */
                        aj->command_name[0] = '\0';
                        size_t cap = sizeof(aj->command_name);
                        size_t used = 0;
                        for (int p = 0; p < cmd_groups[i].pipe_count; p++) {
                            for (int t = 0; t < cmd_groups[i].pipes[p].at_commands; t++) {
                                const char *tok = cmd_groups[i].pipes[p].atomic_commands[t].value;
                                if (!tok) continue;
                                size_t toklen = strlen(tok);
                                size_t avail = (used < cap) ? (cap - used - 1) : 0;
                                size_t to_copy = toklen <= avail ? toklen : avail;
                                if (to_copy > 0) {
                                    memcpy(aj->command_name + used, tok, to_copy);
                                    used += to_copy;
                                    aj->command_name[used] = '\0';
                                }
                                if (used < cap - 1) {
                                    aj->command_name[used] = ' ';
                                    used++;
                                    aj->command_name[used] = '\0';
                                } else goto finish_build_stopped;
                            }
                            if (p < cmd_groups[i].pipe_count - 1) {
                                if (used < cap - 2) {
                                    memcpy(aj->command_name + used, "| ", 2);
                                    used += 2;
                                    aj->command_name[used] = '\0';
                                } else if (used < cap - 1) {
                                    aj->command_name[used] = '|';
                                    used++;
                                    aj->command_name[used] = '\0';
                                } else goto finish_build_stopped;
                            }
                        }
                    finish_build_stopped:
                        while (used > 0 && aj->command_name[used - 1] == ' ') {
                            used--;
                            aj->command_name[used] = '\0';
                        }

                        ////////////////LLM Generated Code Ends//////////////
                        printf("[%d] Stopped %s\n", aj->job_id, aj->command_name);
                        job_count++;
                    } else {
                        fprintf(stderr, "Max background jobs reached.\n");
                    }
                }

                /* foreground finished or moved to background; clear fg pgid */
                current_fg_pgid = 0;
            }
            else
            {
                if(job_count < MAX_JOBS)
                {
                    active_jobs[job_count].pid = representative_pid;
                    active_jobs[job_count].job_id = get_next_job_number();
                    strncpy(active_jobs[job_count].state, "Running", sizeof(active_jobs[job_count].state) - 1);
                    active_jobs[job_count].state[sizeof(active_jobs[job_count].state) - 1] = '\0';
                    char tmp[MAX_INPUT];
                    strcpy(tmp, input);
                    int len = strlen(tmp);
                    while (len > 0 && (tmp[len-1] == '&' || tmp[len-1] == ' ')) {
                        tmp[len-1] = '\0';
                        len--;
                    }
                    
                    ///////////////LLM Generated Code Begins//////////////
                    /* Safely reconstruct the command string into the bounded buffer. */
                    active_jobs[job_count].command_name[0] = '\0';
                    size_t cap = sizeof(active_jobs[job_count].command_name);
                    size_t used = 0;
                    for (int p = 0; p < cmd_groups[i].pipe_count; p++) {
                        for (int t = 0; t < cmd_groups[i].pipes[p].at_commands; t++) {
                            const char *tok = cmd_groups[i].pipes[p].atomic_commands[t].value;
                            if (!tok) continue;
                            size_t toklen = strlen(tok);
                            /* how many bytes we can copy (leave room for NUL) */
                            size_t avail = (used < cap) ? (cap - used - 1) : 0;
                            size_t to_copy = toklen <= avail ? toklen : avail;
                            if (to_copy > 0) {
                                memcpy(active_jobs[job_count].command_name + used, tok, to_copy);
                                used += to_copy;
                                active_jobs[job_count].command_name[used] = '\0';
                            }
                            /* add a separating space if room */
                            if (used < cap - 1) {
                                active_jobs[job_count].command_name[used] = ' ';
                                used++;
                                active_jobs[job_count].command_name[used] = '\0';
                            } else {
                                goto finish_build_cmd;
                            }
                        }
                        if (p < cmd_groups[i].pipe_count - 1) {                            /* append "| " if possible, else append '|' if that fits */
                            if (used < cap - 2) {
                                memcpy(active_jobs[job_count].command_name + used, "| ", 2);
                                used += 2;
                                active_jobs[job_count].command_name[used] = '\0';
                            } else if (used < cap - 1) {
                                active_jobs[job_count].command_name[used] = '|';
                                used++;
                                active_jobs[job_count].command_name[used] = '\0';
                            } else {
                                goto finish_build_cmd;
                            }
                        }
                    }
                finish_build_cmd:
                    /* Trim trailing space */
                    while (used > 0 && active_jobs[job_count].command_name[used - 1] == ' ') {
                        used--;
                        active_jobs[job_count].command_name[used] = '\0';
                    }
                    /////////////// LLM Generated Code Ends ////////////////
                    printf("[%d] %d\n", active_jobs[job_count].job_id, active_jobs[job_count].pid);
                    job_count++;
                    fprintf(stderr, "DEBUG: Added job '%s' with PID %d. Total jobs: %d\n", active_jobs[job_count-1].command_name, active_jobs[job_count-1].pid, job_count);
                }
                 else {
                   fprintf(stderr, "Max background jobs reached.\n");
                    }
            }
        }
        else { 
                 for (int k = 0; k < child_count; k++) {
                    int status;
                    waitpid(child_pids[k], &status, 0);
                 }
            }
             free(child_pids);
        }

        // return;
}

int main()
{
    char history[15][MAX_INPUT];
    for(int i=0; i<15; i++)
    {
        strcpy(history[i], "\0");
    }
    char home[PATH_MAX];
    getcwd(home, sizeof(home));
    long long int home_size = 0;
    while (home[home_size] != '\0')
        home_size++;

    char prompt[LOGIN_NAME_MAX + HOST_NAME_MAX + PATH_MAX + 16];

    char prev[PATH_MAX];
    strcpy(prev, "\0");
    //strcpy(prev, home);

    //log init
    int history_count = 0;
    char history_buffer[15][MAX_INPUT];
    int history_size = 0;
    for(int i=0; i<15; i++)
    {
        strcpy(history_buffer[i], "\0");
    }
    
    char history_path[PATH_MAX+20];
    snprintf(history_path, sizeof(history_path), "%s/history.txt", home); 
    FILE *f_hist = fopen(history_path, "a+");

            if (!f_hist) 
            { 
                perror("fopen");
                exit(1);
            }

    load_from_file(history_path, f_hist, history_buffer, &history_count, &history_size);

    shell_pgid = getpgrp();
    while (tcgetpgrp(STDIN_FILENO) != shell_pgid) {
        kill(-shell_pgid, SIGTTIN);
    }

    //////////////LLM Generated Code Begins//////////
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    struct sigaction sa_int = {0};
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_tstp = {0};
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    /////////////// LLM Generated Code Ends ////////////////
    shell_pgid = getpid();
    if (getpgrp() != shell_pgid && setpgid(shell_pgid, shell_pgid) < 0) {
        perror("setpgid");
        exit(1);
    }
    

    tcsetpgrp(STDIN_FILENO, shell_pgid);


    while (1)
    {
        refresh_jobs();
        build_prompt(prompt, home, home_size);
        printf("%s ", prompt);
        fflush(stdout);

        char input[MAX_INPUT];
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("logout\n");
            fflush(stdout);
            kill_all_jobs();   
            fclose(f_hist);    
            exit(0);           
        }
        // int c = 0;
        // while (input[c] != '\n')
        //     c++;
        // input[c] = '\0';

        input[strcspn(input, "\n")] = '\0';

        if (strspn(input, " \t\r") == strlen(input)) {
            continue;
        }

        token *token_list = (token *)malloc(MAX_INPUT * sizeof(token));
        int tk_size = 0;
        tokenise(input, token_list, &tk_size);

        

        command *cmd_groups = (command *)malloc(MAX_INPUT * sizeof(command));
        int commands = 0;
        for (int i = 0; i < MAX_INPUT; i++)
        {
            cmd_groups[i].pipe_count = 0;
            cmd_groups[i].is_amp = 0;
            for (int j = 0; j < MAX_INPUT; j++)
            {
                cmd_groups[i].pipes[j].at_commands = 0;
            }
        }

        // if (parse_shell_command(token_list, tk_size, cmd_groups, &commands) == 0 || input[0] == ' ' || input[0] == '\t' || input[0] == '\r' || input[0] == '\0')
        // {
        //     if(strcmp(input, history_buffer[(history_count-1)%15])!=0)
        //     add_log(f_hist, history_path, input, &history_size, &history_count, history_buffer);

        //     printf("Invalid Syntax!\n");
        //     continue;
        // }

        if (parse_shell_command(token_list, tk_size, cmd_groups, &commands) == 0)
        {
            if(history_count == 0 || strcmp(input, history_buffer[(history_count-1)%15])!=0)
            add_log(f_hist, history_path, input, &history_size, &history_count, history_buffer);

            printf("Invalid Syntax!\n");
            free(token_list);
            free(cmd_groups);
            continue;
        }


        if(should_log(input, cmd_groups, commands, history_buffer, history_count))
        {
         add_log(f_hist, history_path, input, &history_size, &history_count, history_buffer);
        }

        execute_command(history_path, f_hist, input, cmd_groups, commands, tk_size, home, prev, &history_count, &history_size, history_buffer, 0);

        // fflush(stdout);

        

        free(token_list);
        free(cmd_groups);
    }
    return 0;
}