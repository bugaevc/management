#include <sys/types.h>

#include <unistd.h>
#include <signal.h>

#include "server.h"

struct child {
    pid_t pid;
    struct child *next;
} *children;

enum { MAX_PORT_CNT = 1024 };

int players_cnt;
int ports[MAX_PORT_CNT];
int ports_cnt;

void broadcast_to_children(char *text)
{
    for (struct child *ch = children; ch != NULL; ch = ch->next) {
        kill(ch->pid, SIGUSR2);
        fprintf(pipes.to_children, "%s\n", text);
        pause(); // wait for the child to read the message
    }
}

int parse_cmdline_args(int argc, char *argv[])
{
    if (argc < 2) {
        return 0;
    }
    if (sscanf(argv[1], "%d", &players_cnt) != 1) {
        return 0;
    }
    for (int i = 0; i + 2 < argc; i++) {
        if (sscanf(argv[i + 2], "%d", &ports[i]) != 1) {
            return 0;
        }
    }
    ports_cnt = argc - 2;
    return 1;
}

int main(int argc, char *argv[])
{
    if (!parse_cmdline_args(argc, argv)) {
        fprintf(stderr, "Usage: server players_cnt [port]*\n");
        return 1;
    }
    return 0;
}