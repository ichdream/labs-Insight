#ifndef MMAPCOPY
#define MMAPCOPY

#include<stdio.h>
#include<unistd.h>
#include<sys/mman.h>
#include <stddef.h>
#include<sys/stat.h>
#include<sys/types.h>
#include <fcntl.h>

/* int mmapcopy(char *file); */
int mmapcopy(int fd, int size);

#endif
