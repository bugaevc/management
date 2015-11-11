#include <sys/types.h>

#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"

struct child {
    pid_t pid;
    struct child *next;
} *children;

int players_cnt;
int ports[MAX_PORT_CNT];
int ports_cnt;

void broadcast_to_children(char *text)
{
    for (struct child *ch = children; ch != NULL; ch = ch->next) {
        kill(ch->pid, SIGUSR2);
        fprintf(pipes.to_children_w, "%s\n", text);
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

void setup_signal_handlers(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGUSR2, &act, NULL);
}

void setup_pipes(void)
{
    int pipefd[2][2];
    pipe(pipefd[0]);
    pipe(pipefd[1]);
    pipes.to_children_r = fdopen(pipefd[0][0], "r");
    pipes.to_children_w = fdopen(pipefd[0][1], "w");
    pipes.to_parent_r = fdopen(pipefd[1][0], "r");
    pipes.to_parent_w = fdopen(pipefd[1][1], "w");
}

int main(int argc, char *argv[])
{
    if (!parse_cmdline_args(argc, argv)) {
        fprintf(stderr, "Usage: server players_cnt [port]*\n");
        return 1;
    }
    setup_signal_handlers();
    setup_pipes();
    return 0;
}
