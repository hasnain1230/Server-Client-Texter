//
// Created by lucidity on 8/4/22.
//
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>

// https://man7.org/linux/man-pages/man3/getaddrinfo.3.html

struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

int server(char *port);
void *listener(void *args);

int main() {
   server("587106");
}
/*
 * getaddrinfo gives socket based on host name and port
 * getnameinfo gives host and port based on socket
 */
int server(char *port) { // Port is required to be a string for some dumb reason (no, it actually makes sense because some ports can be really long)
    struct addrinfo hint, *info_list;
    struct addrinfo *head;
    struct connection *con;
    pthread_t tid;
    int socket_fd;

    memset(&hint, 0, sizeof(struct addrinfo));

    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM; // TCP vs UDP <---- We are using TCP here. There are also communication protocols
    hint.ai_flags = AI_PASSIVE; // We are listening for a connection here.

    int error = getaddrinfo(NULL, port, &hint,
                            &info_list); // Since this is the server, we do not need to give it a name since it's on local host
    // info_list returns information in a linked list about the needed to connect to the node+service.
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    for (head = info_list; head != NULL; head = head->ai_next) {
        socket_fd = socket(head->ai_family, head->ai_socktype, head->ai_protocol);

        if (socket_fd == -1) {
            continue;
        }

        if ((bind(socket_fd, head->ai_addr, head->ai_addrlen)) == 0 && (listen(socket_fd, 2) == 0)) {
            break;
        }

        close(socket_fd);
    }

    if (head == NULL) {
        // we reached the end of result without successfully binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(info_list); // Every library in C should have something like this built in
    puts("Waiting for connection");

    con = malloc(sizeof(struct connection));

    while (true) {
        con->fd = accept(socket_fd, (struct sockaddr *) &con->addr, &con->addr_len);

        if (con->fd == -1) {
            perror("accept");
            continue;
        }

        // spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, listener, &con);

        // if we couldn't spin off the thread, clean up and wait for another connection
        if (error != 0) {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            // free(con);
            continue;
        }

        // otherwise, detach the thread and wait for the next connection request
        pthread_detach(tid);
    }

    // never reach here
    return 0;
}

void *listener(void *args) {
    int numberRead;
    char host[256], port[32], buffer[9]; // One more for null terminator
    struct connection *con = (struct connection *) args;

    int error = getnameinfo((struct sockaddr *) &con->addr, con->addr_len, host, 256, port, 9, NI_NUMERICSERV);

    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(con->fd);
        return NULL;
    }

    printf("[%s:%s] connection\n", host, port);

    while ((numberRead = read(con->fd, buffer, 8)) > 0) {
        buffer[numberRead] = '\0';
        printf("[%s:%s] read %d bytes |%s|\n", host, port, numberRead, buffer);
    }

    printf("[%s:%s] got EOF\n", host, port);

    close(con->fd);
    free(con);
    return NULL;
}
