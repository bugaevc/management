#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>

#include "server.h"

struct child {
    pid_t pid;
    struct child *next;
} *children;

void broadcast_to_children(char *text)
{
    for (struct child *ch = children; ch != NULL; ch = ch->next) {
        kill(ch->pid, SIGUSR2);
        fprintf(pipes->to_children_w, "%s\n", text);
        pause(); // wait for the child to read the message
    }
}

int parse_cmdline_args(size_t argc, char *argv[], int *players_cnt, struct vector *ports)
{
    if (argc < 2) {
        return 0;
    }
    if (sscanf(argv[1], "%d", players_cnt) != 1) {
        return 0;
    }
    for (size_t i = 0; i + 2 < argc; i++) {
        if (sscanf(argv[i + 2], "%d", &ports->arr[i]) != 1) {
            return 0;
        }
    }
    ports->cnt = argc - 2;
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
    pipes = malloc(sizeof(*pipes));
    pipes->to_children_r = fdopen(pipefd[0][0], "r");
    pipes->to_children_w = fdopen(pipefd[0][1], "w");
    pipes->to_parent_r = fdopen(pipefd[1][0], "r");
    pipes->to_parent_w = fdopen(pipefd[1][1], "w");
    setvbuf(pipes->to_children_r, NULL, _IONBF, 0);
    setvbuf(pipes->to_children_w, NULL, _IONBF, 0);
    setvbuf(pipes->to_parent_r, NULL, _IONBF, 0);
    setvbuf(pipes->to_parent_w, NULL, _IONBF, 0);
}

void setup_sockets(struct vector *ports, struct vector *socks)
{
    for (size_t i = 0; i < ports->cnt; i++) {
        socks->arr[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (socks->arr[i] < 0) {
            perror("socket");
            exit(2);
        }
        union {
            struct sockaddr sa;
            struct sockaddr_in sa_in;
        } sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_in.sin_family = AF_INET;
        sa.sa_in.sin_port = htons(ports->arr[i]);
        sa.sa_in.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(socks->arr[i], &sa.sa, sizeof(sa)) < 0) {
            perror("bind");
            exit(2);
        }
        if (listen(socks->arr[i], 5) < 0) {
            perror("listen");
            exit(2);
        }
    }
    socks->cnt = ports->cnt;
}

// Close&free all the parent stuff
void init_child(struct vector *socks)
{
    for (struct child *ch = children, *next = children->next; ch != NULL; ch = next) {
        free(ch);
    }
    for (size_t i = 0; i < socks->cnt; i++) {
        close(socks->arr[i]);
    }
    fclose(pipes->to_children_w);
    fclose(pipes->to_parent_r);
}

void wait_for_players(struct vector *socks, int players_cnt)
{
    int current_players_cnt = 0;
    while (current_players_cnt < players_cnt) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_d = -1;
        for (size_t i = 0; i < socks->cnt; i++) {
            if (socks->arr[i] > max_d) {
                max_d = socks->arr[i];
            }
            FD_SET(socks->arr[i], &read_fds);
        }
        // this will block
        int res = select(max_d + 1, &read_fds, NULL, NULL, NULL);
        if (res < 1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            exit(2);
        }
        for (size_t i = 0; i < socks->cnt; i++) {
            if (FD_ISSET(socks->arr[i], &read_fds)) {
                // new connection!
                int new_fd = accept(socks->arr[i], NULL, NULL);
                if (new_fd < 0) {
                    perror("accept");
                    // do not abort the program
                    // continue to the next iteration of the main loop
                    break;
                }
                current_players_cnt++;
                // FIXME: write something nice
                broadcast_to_children("New connection!");
                // prepend a new struct child to the list
                struct child *tmp = children;
                children = malloc(sizeof(struct child));
                children->next = tmp;
                children->pid = fork();
                if (!children->pid) {
                    // Screw that, we don't need that structure anymore
                    // Because we're the child!
                    // Close&free everithing
                    init_child(socks);
                    run_child(new_fd, players_cnt, current_players_cnt);
                    exit(0);
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int players_cnt;
    struct vector *ports = malloc(sizeof(*ports));
    if (!parse_cmdline_args(argc, argv, &players_cnt, ports)) {
        fprintf(stderr, "Usage: %s players_cnt [port]*\n", argv[0]);
        return 1;
    }
    parent_pid = getpid();
    setup_signal_handlers();
    setup_pipes();
    struct vector *socks = malloc(sizeof(*socks));
    setup_sockets(ports, socks);
    wait_for_players(socks, players_cnt);
    printf("Game here");
    return 0;
}
