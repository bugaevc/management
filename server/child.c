#include <sys/types.h>

#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "server.h"

FILE *client_socket;

void no_news_handler(__attribute__((unused)) int signum)
{
    fprintf(pipes.to_parent_w, "No news\n");
}

void broadcast_handler(__attribute__((unused)) int signum)
{
    char text[MAX_MESSAGE_LEN + 2];
    fgets(text, sizeof(text), pipes.to_children_r);
    fprintf(client_socket, "%s", text);
    kill(parent_pid, SIGUSR2);
}

void send_to_parent(char *text)
{
    struct sigaction old_act, new_act;
    memset(&new_act, 0, sizeof(new_act));
    new_act.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &new_act, &old_act);
    kill(parent_pid, SIGUSR1); // notify the parent_pid
    pause(); // wait for it to ask us
    sigaction(SIGUSR1, &old_act, NULL);
    fprintf(pipes.to_parent_w, "%s\n", text);
}
