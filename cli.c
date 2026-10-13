/* Interactive client for the linked list TCP server. */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PORT 9001
#define MAX_COMMAND_LINE_LEN 1024
int main(void) {
    signal(SIGPIPE, SIG_IGN);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return EXIT_FAILURE; }
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect"); close(sock); return EXIT_FAILURE;
    }
    char command[MAX_COMMAND_LINE_LEN];
    int status = EXIT_SUCCESS;
    for (;;) {
        printf("Enter Command (or menu): ");
        fflush(stdout);
        if (!fgets(command, sizeof(command), stdin)) break;
        size_t length = strlen(command);
        if (length && command[length - 1] == '\n') command[--length] = '\0';
        else if (!feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("Command too long"); continue;
        }
        if (strspn(command, " \t\r") == length) continue;
        if (!strcmp(command, "menu")) {
            puts("COMMANDS:\n---------\n1. print\n2. get_length\n3. add_back <value>\n4. add_front <value>\n5. add_position <index> <value>\n6. remove_back\n7. remove_front\n8. remove_position <index>\n9. get <index>\n10. exit");
            continue;
        }
        size_t sent = 0;
        while (sent < length) {
            ssize_t n = send(sock, command + sent, length - sent, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) { perror("send"); status = EXIT_FAILURE; goto cleanup; }
            sent += (size_t)n;
        }
        if (!strcmp(command, "exit")) break;
        printf("\nSERVER RESPONSE: ");
        /* Read through the terminating NUL, including split TCP replies. */
        for (;;) {
            char ch;
            ssize_t n = recv(sock, &ch, 1, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) {
                if (n < 0) perror("recv");
                status = EXIT_FAILURE; goto cleanup;
            }
            if (!ch) break;
            putchar(ch);
        }
        putchar('\n');
    }
cleanup:
    close(sock);
    return status;
}
