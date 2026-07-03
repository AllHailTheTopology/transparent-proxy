#define _GNU_SOURCE
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROXY_PORT 6666
#define PROXY_TARGET_PORT 8080
#define PROXY_TARGET_IP 0x0a000601
#define MAX_MSG_SIZE 1024 * 1024

int proxy_do(int client_sock) {
    struct sockaddr_in server_addr = {.sin_addr = htonl(PROXY_TARGET_IP),
                                      .sin_port = htons(PROXY_TARGET_PORT),
                                      .sin_family = AF_INET};

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int pipe[2];
    pipe2(pipe, O_NONBLOCK);

    if (server_sock < 0) {
        perror("proxy_do: socket");
        return 0;
    }

    if (connect(server_sock, (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("proxy_do: connect");
        goto cleanup;
    }

    // client -> server via splice pipe
    if (splice(client_sock, 0, pipe[1], 0, MAX_MSG_SIZE, SPLICE_F_MOVE) < 0) {
        perror("proxy_do: client splice to pipe");
        goto cleanup;
    }
    if (splice(pipe[0], 0, server_sock, 0, MAX_MSG_SIZE, SPLICE_F_MOVE) < 0) {
        perror("proxy_do: pipe spice to server");
        goto cleanup;
    }

    // client <- server via splice pipe
    if (splice(server_sock, 0, pipe[1], 0, MAX_MSG_SIZE, SPLICE_F_MOVE) < 0) {
        perror("proxy_do: server splice");
        goto cleanup;
    }
    if (splice(pipe[0], 0, client_sock, 0, MAX_MSG_SIZE, SPLICE_F_MOVE) < 0) {
        perror("proxy_do: pipe spice to client");
        goto cleanup;
    }

cleanup:
    close(server_sock);
    close(pipe[0]);
    close(pipe[1]);
    return 0;
}

int main(int argc, char* argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {
        .sin_addr = 0, .sin_port = htons(PROXY_PORT), .sin_family = AF_INET};
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("proxy start *:%d \n", PROXY_PORT);

    while (1) {
        struct sockaddr_in client = {0};
        socklen_t addr_n = sizeof(client);
        char buf[INET_ADDRSTRLEN];

        int client_sock = accept(sock, (struct sockaddr*)&client, &addr_n);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        if (inet_ntop(AF_INET, &client.sin_addr, buf, sizeof(buf)) == NULL) {
            perror("inet_ntop");
            continue;
        }

        printf("new client: %s\n", buf);
        proxy_do(client_sock);
        close(client_sock);
    }
}
