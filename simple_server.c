#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>


int main() {
    // 创建监听fd
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 填充sockaddr_in结构
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(2048);

    // sockfd绑定ip，port
    if (-1 == bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        perror("bind");
        return -1;
    }

    // sockfd进入监听状态
    listen(sockfd, 10);

    // accept接收客户端请求，创建clientfd对应客户端
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
    printf("accept\n");

#if 0       // 只能接收一次数据
    char buffer[128] = {0};
    int count = recv(clientfd, buffer, 128, 0);
    send(clientfd, buffer, count, 0);
    printf("sockfd: %d, clientfd: %d, count: %d, buffer: %s\n", sockfd, clientfd, count, buffer);

#else    // 循环接收，能接收多次数据
    while (1) {
        char buffer[128] = {0};
        int count = recv(clientfd, buffer, 128, 0);
        if (count == 0) {
            printf("disconnect\n");
            break;
        }
        send(clientfd, buffer, count, 0);
        printf("sockfd: %d, clientfd: %d, count: %d, buffer: %s\n", sockfd, clientfd, count, buffer);
    }
#endif

    // 服务端阻塞，然后断开
    getchar();
    close(clientfd);

    return 0;
}