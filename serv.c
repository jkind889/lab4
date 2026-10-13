/* TCP command server for the Lab 1 linked list interface. */
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#define PORT 9001
static volatile sig_atomic_t stopping = 0;
static int listener = -1, client = -1;
static void stop_server(int signal_number) {
    (void)signal_number;
    stopping = 1;
    if (client >= 0) close(client);
    if (listener >= 0) close(listener);
}
static int number(const char *text, int *value) {
    if (!text) return 0;
    char *end;
    errno = 0;
    long n = strtol(text, &end, 10);
    if (errno || *end || end == text || n < INT_MIN || n > INT_MAX) return 0;
    *value = (int)n;
    return 1;
}
/* Send the whole response even if TCP accepts only part at a time. */
static int reply(const char *text) {
    size_t sent = 0, length = strlen(text) + 1;
    while (sent < length) {
        ssize_t n = send(client, text + sent, length - sent, 0);
        if (n < 0 && errno == EINTR && !stopping) continue;
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}
int main(void) {
    struct sigaction action = {0};
    action.sa_handler = stop_server;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    signal(SIGPIPE, SIG_IGN);
    list_t *mylist = list_alloc();
    if (!mylist) return EXIT_FAILURE;
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); list_free(mylist); return EXIT_FAILURE; }
    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    int status = EXIT_SUCCESS;
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(listener, 1) < 0) {
        perror("server setup"); status = EXIT_FAILURE; goto cleanup;
    }
    client = accept(listener, NULL, NULL);
    if (client < 0) { if (!stopping) { perror("accept"); status = EXIT_FAILURE; } goto cleanup; }
    while (!stopping) {
        char buffer[1024], response[128];
        ssize_t received = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (received < 0 && errno == EINTR && !stopping) continue;
        if (received <= 0) break;
        buffer[received] = '\0';
        char *command = strtok(buffer, " \t\r\n");
        if (!command) { if (!reply("ERROR")) break; continue; }
        char *arg1 = strtok(NULL, " \t\r\n");
        char *arg2 = strtok(NULL, " \t\r\n");
        char *extra = strtok(NULL, " \t\r\n");
        int first = 0, second = 0, value = 0;
        strcpy(response, "ERROR");
        if (!strcmp(command, "exit") && !arg1) break;
        if (!strcmp(command, "print") && !arg1) {
            char *text = listToString(mylist);
            int ok = reply(text ? text : "ERROR");
            free(text);
            if (!ok) break;
            continue;
        } else if (!strcmp(command, "get_length") && !arg1) {
            snprintf(response, sizeof(response), "Length = %d", list_length(mylist));
        } else if ((!strcmp(command, "add_front") || !strcmp(command, "add_back")) && number(arg1, &first) && !arg2) {
            if (!strcmp(command, "add_front")) list_add_to_front(mylist, first);
            else list_add_to_back(mylist, first);
            snprintf(response, sizeof(response), "ACK%d", first);
        } else if (!strcmp(command, "add_position") && number(arg1, &first) && number(arg2, &second) && !extra) {
            if (first >= 1 && first <= list_length(mylist) + 1) {
                list_add_at_index(mylist, second, first);
                snprintf(response, sizeof(response), "ACK%d", second);
            }
        } else if ((!strcmp(command, "remove_front") || !strcmp(command, "remove_back")) && !arg1) {
            value = !strcmp(command, "remove_front") ? list_remove_from_front(mylist) : list_remove_from_back(mylist);
            snprintf(response, sizeof(response), "ACK%d", value);
        } else if ((!strcmp(command, "remove_position") || !strcmp(command, "get")) && number(arg1, &first) && !arg2) {
            value = !strcmp(command, "get") ? list_get_elem_at(mylist, first) : list_remove_at_index(mylist, first);
            snprintf(response, sizeof(response), "ACK%d", value);
        } else if (!strcmp(command, "menu") && !arg1) {
            strcpy(response, "ACK");
        }
        if (!reply(response)) break;
    }
cleanup:
    list_free(mylist);
    if (!stopping) { if (client >= 0) close(client); if (listener >= 0) close(listener); }
    return status;
}
