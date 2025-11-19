#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include "tokens.h"

int ascii_sort(const struct dirent **a, const struct dirent **b) {
    return strcmp((*a)->d_name, (*b)->d_name);
}

void list_order(char* path, int a, int l)
{
    struct dirent **des;
    int n = scandir(path, &des, NULL, ascii_sort);

    if(n==-1)
    {
        fprintf(stderr, "No such directory!\n");
        return;
    }

    for(int i=0; i<n; i++)
    {
            if(a==1 || (a==0 && des[i]->d_name[0]!='.'))
            {
            printf("%s", des[i]->d_name);
            if(l==1)
            printf("\n");
            else
            printf(" ");
            }
    }

    if(!l)
    printf("\n");

}

void reveal_process(token* at_tk_list, int tk_size, char* home, char* prev)
{
    if(tk_size==1)
    {
      list_order(".", 0, 0);
      return;
    }

    int path_mentioned = 0;
    char* last_token = (char*)malloc(4096*sizeof(char));
    strcpy(last_token, at_tk_list[tk_size-1].value);
    int sz = 0;
    if(strcmp(last_token, "~")==0 || strcmp(last_token, ".")==0 || strcmp(last_token, "..")==0 || strcmp(last_token, "-")==0)
    path_mentioned = 1;
    else {
        while(last_token[sz]!='\0')
        {
            char c = last_token[sz];
            if(sz==0 && c=='-')
            {
             sz++;
            continue;
            }
            if(c!='a' && c!='l')
            {
            path_mentioned = 1;
            break;
            }

            sz++;
        }
    }

    if (path_mentioned && tk_size > 2) {
        for (int i = 1; i < tk_size - 1; i++) {
            if (at_tk_list[i].value[0] != '-') {
                fprintf(stderr, "reveal: Invalid Syntax!\n");
                return;
            }
        }
    } else if(!path_mentioned) {
        for (int i = 1; i < tk_size; i++) {
            if (at_tk_list[i].value[0] != '-') {
                fprintf(stderr, "reveal: Invalid Syntax!\n");
                return;
            }
        }
    }
    
    
    

    int l = 0; //l absent till now
    int a = 0; //a absent till now

    for(int i=1; i<tk_size; i++)
    {
        if(i==tk_size-1 && path_mentioned)
        continue;

        int sz = 0;
        char* last_token = (char*)malloc(4096*sizeof(char));
        strcpy(last_token, at_tk_list[i].value);
        while(last_token[sz]!='\0' && !(a==1 && l==1))
        {
            char c = last_token[sz];
            if(c=='a')
            a=1;
            else if(c=='l')
            l=1;

            sz++;
        }

        
    }

    if(!path_mentioned || (path_mentioned && strcmp(at_tk_list[tk_size-1].value, ".")==0))
    {
        list_order(".", a, l);
    }

    else {
        char current_path[PATH_MAX];
        getcwd(current_path, PATH_MAX);

        char* token = (char*)malloc(4096*sizeof(char));
        strcpy(token, at_tk_list[tk_size-1].value);

        

        if(strcmp("~", token)==0)
        {
            list_order(home, a, l);
        }

        else if(strcmp(token, "..")==0)
        {
            list_order("..", a, l);
        }

        else if(strcmp(token, "-")==0)
        {
            list_order(prev, a, l);
            // chdir(current_path);
        }

        else {
            list_order(token, a, l);
            // chdir(current_path);
        }
    }

return;
}