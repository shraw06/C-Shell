#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "tokens.h"
#include "hop.h"

void hop_process(token* at_tk_list, int tk_size, char* home, char* prev)
{
                    if(tk_size==1)
                    {
                       getcwd(prev, PATH_MAX);
                       if(chdir(home)==-1)
                            {
                                printf("home not found. :|\n");
                                exit(1);
                            } 
                    }
                    for(int k=1; k<tk_size; k++)
                    {
                        if(strcmp(at_tk_list[k].value, ".")==0)
                        continue;

                        else if(strcmp(at_tk_list[k].value, "~")==0)
                        {
                            getcwd(prev, PATH_MAX);
                            if(chdir(home)==-1)
                            {
                                printf("home not found. :|\n");
                                exit(1);
                            }
                        }

                        else if(strcmp(at_tk_list[k].value, "..")==0)
                        {
                            getcwd(prev, PATH_MAX);
                            char current_path[PATH_MAX];
                            getcwd(current_path, sizeof(current_path));
                            int slash_index=0;
                            for(int i=0; current_path[i]!='\0'; i++)
                            {
                                if(current_path[i]=='/')
                                slash_index = i;
                            }

                            if(slash_index > 0)
                            current_path[slash_index]='\0';
                            

                            if(chdir(current_path)==-1)
                            {
                                fprintf(stderr, "No such directory!\n");
                            }

                        }

                        else if(strcmp(at_tk_list[k].value, "-")==0)
                        {
                            char prev_path[PATH_MAX];
                            getcwd(prev_path, sizeof(prev_path));
                            if(strcmp(prev, "\0")==0)
                            {
                                continue;
                            }
                            if(chdir(prev)==-1)
                            {
                                fprintf(stderr, "No such directory!\n");
                            }
                            strcpy(prev, prev_path);
                        }
          
                        else {
                            getcwd(prev, PATH_MAX);
                            if(chdir(at_tk_list[k].value)==-1)
                            {
                                fprintf(stderr, "No such directory!\n");
                            }
                        }
                    } 
}



