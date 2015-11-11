#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>

struct {
    FILE *to_children, *to_parent;
} pipes;


pid_t parent_pid;

#endif