//
// Created by lucidity on 8/4/22.
//
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

// https://man7.org/linux/man-pages/man3/getaddrinfo.3.html

// Server:
// Get addrinfo for information to create a socket FD
// Create listening socket
// Bind to port
// Listen and accept connections
// Read and write into file descriptor

struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

int server(char *port);
void *listener(void *args);

int main(int argc, char *argv[]) {
    // Port 587106
    server(argv[1]); // This port can really be anything > 1024. Ports < 1024 are reserved for major network related applications running on your system. Must be root to bind and listen on ports less than this. Don't do this, that's dangerous unless you know what
                            // you are doing.
}
/*
 * getaddrinfo gives socket based on host name and port
 * getnameinfo gives host and port based on socket
 */
int server(char *port) { // Port is required to be a string for some dumb reason (no, it actually makes sense because some ports can be really long)
    int socket_fd;
    struct addrinfo hint, *info_list, *head;
    struct connection *con;
    pthread_t tid;

    memset(&hint, 0, sizeof(struct addrinfo));

    hint.ai_family = AF_UNSPEC; // Can be IPV6 or IPV4. There may be problems with this, but in general it's fine
    hint.ai_socktype = SOCK_STREAM; // TCP vs UDP <---- We are using TCP here. There are also communication protocols
    hint.ai_flags = AI_PASSIVE; // We are listening for a connection here.

    int error = getaddrinfo(NULL, port, &hint,
                            &info_list); // Since this is the server, we do not need to give it a name since it's on local host
    // info_list returns information in a linked list about the needed to connect to the node+service.
    if (error != 0) {
        fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(error));
        return -1;
    }

    for (head = info_list; head != NULL; head = head->ai_next) {
        socket_fd = socket(head->ai_family, head->ai_socktype, head->ai_protocol);

        if (socket_fd == -1) {
            continue;
        }

        if ((bind(socket_fd, head->ai_addr, head->ai_addrlen) == 0) && (listen(socket_fd, 8) == 0)) {
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

    while (true) {
        con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        con->fd = accept(socket_fd, (struct sockaddr *) &con->addr, &con->addr_len);

        if (con->fd == -1) {
            perror("accept error");
            continue;
        }

        // Spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, listener, con);

        if (error != 0) {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            // free(con);
            continue;
        }

        // Wait for next connection request
        pthread_detach(tid);
    }
}

void *listener(void *args) {
    int bytesRead;
    char host[256], port[32], buffer[9]; // One more for null terminator
    struct connection *con = (struct connection *) args;

    int error = getnameinfo((struct sockaddr *) &con->addr, con->addr_len, host, 256, port, 8, NI_NUMERICSERV); // Give us an actual port number

    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(con->fd);
        return NULL;
    }

    printf("[%s:%s] connection\n", host, port);

    while ((bytesRead = read(con->fd, buffer, 8)) > 0) { // Read blocks by default if nothing is available
        buffer[bytesRead] = '\0';
        printf("[%s:%s] read %d bytes |%s|\n", host, port, bytesRead, buffer);
    }

    printf("[%s:%s] got EOF\n", host, port);

    close(con->fd);
    free(con);
    return NULL;
}
