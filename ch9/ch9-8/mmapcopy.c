#include "mmapcopy.h"

/*
int mmapcopy(char* file)
{
    if(file == NULL) {
        fprintf(stderr, "Can't open it!\n");
        return -1;
    }
    FILE *fp = fopen(file, "r" );
    while(!feof(fp)) {
        char buff[255];
        if(fgets(buff, 255, fp)) {
            fprintf(stdout, buff);
        }
    }
    return 0;
}
*/

int mmapcopy(int fd, int size)
{
    char *bufp;
    bufp = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    write(1, bufp, size);
    return 0;
}
