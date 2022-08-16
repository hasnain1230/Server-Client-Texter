//
// Created by Hasnain Ali on 8/15/22.
//

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>


// Client: Create socket for host you want to connect to.
// Send data by writing into socket FD


void client(char *argv[]) {
    struct addrinfo hints, *res, *res_list;
    int socket_fd;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int error = getaddrinfo(argv[1], argv[2], &hints, &res_list);

    if (error) {
        fprintf(stderr, "%s\n", gai_strerror(error));
        exit(EXIT_FAILURE);
    }

    for (res = res_list; res != NULL; res = res->ai_next) {
        socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

        if (socket_fd < 0) {
            continue;
        }

        if (connect(socket_fd, res->ai_addr, res->ai_addrlen) == 0) {
            break;
        }

        close(socket_fd);
    }

    if (res == NULL) {
        fprintf(stderr, "Could not connect to %s:%s\n", argv[1], argv[2]);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res_list);

    write(socket_fd, argv[3], strlen(argv[3])); // This is slightly unsafe, but fine for demonstration purposes

    close(socket_fd);
}

int main(int argc, char *argv[]) {
    client(argv);
}
