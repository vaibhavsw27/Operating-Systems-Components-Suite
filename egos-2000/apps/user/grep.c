//Vaibhav 2024597 OS BONUS PROJECT

#include "app.h"
#include <string.h>

int main(int argc,char**argv) 
{
    if (argc != 3) 
    {
        INFO("Incorrect input\n");
        return -1;
    }

    char *pattern=argv[1];
    char *filename=argv[2];

    // looking up the inode number of the file
    int ino=dir_lookup(workdir_ino,filename);
    if (ino<0) //error handling
    {
        INFO("grep file not found\n");
        return -1;
    }

    
    char buf[4096];  //max file size can be 4KB
    memset(buf,0,sizeof(buf));

    int offset = 0;
    int total = 0;

    while (1) 
    {
        int block = file_read(ino,offset,buf + total);
        if (block<=0)
        {
            break;
        }
        offset+=BLOCK_SIZE;
        total+=block;

        if (total>= sizeof(buf)-BLOCK_SIZE)
        {
            break;
        }

    }

    char *line=strtok(buf,"\n");
    while (line!=NULL) 
    {
        if (strstr(line,pattern)) 
        {
            INFO("%s\n",line);
        }
        line=strtok(NULL,"\n");
    }

    return 0;
}
