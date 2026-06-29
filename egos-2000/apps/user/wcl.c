//Vaibhav 2024597 OS BONUS PROJECT

#include "app.h"
#include <string.h>

int main(int argc, char **argv) 
{
    if (argc < 2) 
    {
        INFO("Incorrect input\n");
        return -1;
    }

    int total = 0;
    int correction_factor=0;
    for (int i=1;i<argc;i++) 
    {
        char *filename = argv[i];

        int ino = dir_lookup(workdir_ino, filename);

        if (ino < 0) 
        {
            INFO("File %s not found\n", filename);
            continue;
        }
        else
        {
            correction_factor++;
        }

        char buf[BLOCK_SIZE];
        file_read(ino, 0, buf);

        int lines = 0;
        for (int j=0;j<strlen(buf);j++) 
        {
            if (buf[j] == '\n')
            {
                lines++;
            }
        }

        total += lines;
    }

    total+=correction_factor;
    INFO("%d\n", total);

    return 0;
}