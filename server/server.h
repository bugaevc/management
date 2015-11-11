#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>

enum {
    MAX_PORT_CNT = 1024,
    MAX_MESSAGE_LEN = 1024
};

struct {
    FILE *to_children_r, *to_children_w;
    FILE *to_parent_r, *to_parent_w;
} *pipes;

pid_t parent_pid;

#endif
