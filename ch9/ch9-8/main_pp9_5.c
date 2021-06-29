#include<stdio.h>
#include "mmapcopy.h"

int main()
{    
    char file[80];
    printf("input file name: ");
    scanf("%s", file);
    printf("file name: %s\n", file);
    /*if(mmapcopy(file) != -1) {
        printf("write success.\n");
    }
    */
    struct stat stat;
    int fd = open(file, O_RDONLY, 0);
    fstat(fd, &stat);
    mmapcopy(fd, stat.st_size);
    return 0;
}
