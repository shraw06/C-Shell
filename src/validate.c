#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"
#include "validate.h"

int validate_names(char* name)
{
    int i=0;
    while(name[i]!='\0')
    {
        if(name[i]=='&' || name[i]==';' || name[i]=='|' || name[i]=='<' || name[i]=='>')
        return 0;

        i++;
    }
    return 1;
}

int parse_atomic(token* token_list, int tk_size)
{
    if(tk_size==0)
    return 0;

    if(strcmp(token_list[0].value, "<")==0 || strcmp(token_list[0].value, ">")==0 || strcmp(token_list[0].value, ">>")==0)
    return 0;

    if(strcmp(token_list[tk_size-1].value, "<")==0 || strcmp(token_list[tk_size-1].value, ">")==0 || strcmp(token_list[tk_size-1].value, ">>")==0)
    return 0;

    for(int i=0; i<tk_size; i++)
    {
        
        if(strcmp(token_list[i].value, "<")==0 || strcmp(token_list[i].value, ">")==0 || strcmp(token_list[i].value, ">>")==0)
        {
            if(i!=tk_size-1 && (strcmp(token_list[i+1].value, "<")==0 || strcmp(token_list[i+1].value, ">")==0 || strcmp(token_list[i+1].value, ">>")==0))
            return 0;
        
            continue;
        }

        if(validate_names(token_list[i].value)==0)
        return 0;
    }
    return 1;
}

int parse_command_groups(token* token_list, int tk_size, command* cmd_groups, int* commands)
{
    if(tk_size==0)
    return 0;

    if(strcmp(token_list[0].value, "|")==0 || strcmp(token_list[tk_size-1].value, "|")==0)
    return 0;

    for(int i=0; i<tk_size; i++)
    {
        token* atomic_tokens = (token*)malloc(tk_size*sizeof(token));
        int pos = 0;
        while(i<tk_size && strcmp(token_list[i].value, "|")!=0)
        {
            atomic_tokens[pos++] = token_list[i];
            i++;
        }


        if(parse_atomic(atomic_tokens, pos)==0)
        {
        free(atomic_tokens);
        return 0;
        }

        int pipe_id = cmd_groups[*commands].pipe_count;
        // cmd_groups[*commands].pipes[cmd_groups[*commands].pipe_count].atomic_commands = atomic_tokens;
        cmd_groups[*commands].pipes[pipe_id].atomic_commands = malloc(pos * sizeof(token));
        memcpy(cmd_groups[*commands].pipes[pipe_id].atomic_commands, atomic_tokens, pos * sizeof(token));
        cmd_groups[*commands].pipes[cmd_groups[*commands].pipe_count].at_commands = pos;

        free(atomic_tokens);
        cmd_groups[*commands].pipe_count++;

    }

    return 1;
}

int parse_shell_command(token* token_list, int tk_size, command* cmd_groups, int* commands)
{
    if(strcmp(token_list[tk_size-1].value, ";")==0 || strcmp(token_list[0].value, "&")==0 || strcmp(token_list[0].value, ";")==0)
    return 0;

    for(int i=0; i<tk_size; i++)
    {
        token* cmd_grp_tokens = (token*)malloc(tk_size*sizeof(token));
        int pos = 0;
        while(i<tk_size && strcmp(token_list[i].value, "&")!=0 && strcmp(token_list[i].value, ";")!=0)
        {
            cmd_grp_tokens[pos++] = token_list[i];
            i++;
        }


        if(parse_command_groups(cmd_grp_tokens, pos, cmd_groups, commands)==0)
        {
        free(cmd_grp_tokens);
        return 0;
        }

        // cmd_groups[*commands].is_amp = (i<tk_size && strcmp(token_list[i].value, "&")==0? 1:0);
        // cmd_groups[*commands].is_amp = ((i<tk_size && strcmp(token_list[i].value, "&")!=0 && strcmp(token_list[i].value, ";")==0)? 2:0);

        cmd_groups[*commands].is_amp = 0;
        if(i<tk_size)
        {
            if(strcmp(token_list[i].value, "&")==0)
            cmd_groups[*commands].is_amp = 1;
            else if(strcmp(token_list[i].value, ";")==0)
            cmd_groups[*commands].is_amp = 2;
        }
        // 1 for &, 2 for ;, 0 for nothing

        free(cmd_grp_tokens);

        *commands = *commands + 1;
    }


    return 1;
}