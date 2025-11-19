#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"

background_job active_jobs[MAX_JOBS];
int job_count = 0;

void tokenise(char* input, token* token_list, int* tk_size)
{
  int i=0;
  int pos = 0;
  while(input[i]!='\0')
  {
    char c = input[i];
    
    if(c=='&')
    {
        token_list[pos].type = AND;
        strcpy(token_list[pos++].value, "&");
    }
    else if(c==';')
    {
        token_list[pos].type = SEMICOLON;
        strcpy(token_list[pos++].value, ";");
    }
    else if(c=='|')
    {
        token_list[pos].type = PIPE;
        strcpy(token_list[pos++].value, "|");
    }
    else if(c=='<')
    {
        token_list[pos].type = INPUT;
        strcpy(token_list[pos++].value, "<");
    }
    else if(c=='>')
    {
        if(input[i+1]!='>')
        {
            token_list[pos].type = OUTPUT;
            strcpy(token_list[pos++].value, ">");
        }
        else
        {
            token_list[pos].type = APPEND;
            strcpy(token_list[pos++].value, ">>");
            i++;
        }
    }
    else
    {
        char value[4096];
        int pv = 0;
        while(input[i]!='\0' && input[i]!=' ' && input[i]!='\t' && input[i]!='\r' && input[i]!='&' && input[i]!=';' && input[i]!='<' && input[i]!='>' && input[i]!='|')
        {
            value[pv++] = input[i];
            i++;
        }
        value[pv]='\0';
        if(value[0]!='\0')
        {
        strcpy(token_list[pos].value, value);
        token_list[pos++].type = NAME;
        }

        if(input[i]=='&' || input[i]==';' || input[i]=='<' || input[i]=='>' || input[i]=='|')
        i--;
    }
    i++;  
  }
  *tk_size = pos;
}