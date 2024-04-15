#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

void *client_thread(void *arg) {

    int clientfd = *(int *)arg;

    while (1) {
        char buffer[128] = {0};
        int count = recv(clientfd, buffer, 128, 0);
        if (count == 0) {
            printf("disconnect\n");
            break;
        }
        send(clientfd, buffer, count, 0);
        printf("clientfd: %d, count: %d, buffer: %s\n", clientfd, count, buffer);
    }

    close(clientfd);
}


// tcp 
int main() {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(struct sockaddr_in));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(2048);

    if (-1 == bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr))) {
        perror("bind");
        return -1;
    }

    listen(sockfd, 10);

    while (1) {

        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, &clientfd);
    }

    getchar();
}




