#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include "prompt.h"

void build_prompt(char* buffer, char* home, long long int home_size){
    int uid = getuid();
    struct passwd* p = getpwuid(uid);
    char username[LOGIN_NAME_MAX];
    strcpy(username, p->pw_name);
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof(hostname));
    
    
    char current_path[PATH_MAX];
    char path[PATH_MAX];
    getcwd(current_path, sizeof(current_path));

    if(strncmp(current_path, home, home_size)==0 && (current_path[home_size] == '/' || current_path[home_size] == '\0'))
    {
        path[0]='~';
        int j=1;
        for(int i=home_size; current_path[i]!='\0'; i++, j++)
        {
            path[j] = current_path[i];
        }
        path[j] ='\0';
    } 
    else
    strcpy(path, current_path);   

   
    sprintf(buffer, "<%s@%s:%s>", username, hostname, path);
       
}