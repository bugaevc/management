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

struct vector {
    int arr[MAX_PORT_CNT];
    size_t cnt;
};

pid_t parent_pid;

void run_child(int fd, int players_cnt, int current_players_cnt);
void empty_handler(int signum);

#endif
