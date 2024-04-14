#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/poll.h>


int main() {
    // 创建listen fd
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

    // 初始化pollfd数组
    struct pollfd fds[1024] = {0};

    // 将sockfd，关注事件封装成pollfd, 放入pollfd数组，更新maxfd
    fds[sockfd].fd = sockfd;
    fds[sockfd].events = POLLIN;
    int maxfd = sockfd;

    printf("loop\n");

    while (1) {
        int nready = poll(fds, maxfd+1, -1);

        // sockfd fd发生读事件，io可读了
        if (fds[sockfd].revents & POLLIN) {
            struct sockaddr_in clientaddr;
            socklen_t len = sizeof(clientaddr);
            
            int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

            printf("sockfd: %d\n", clientfd);

            // 将client fd放入pollfd数组，关注读事件，更新maxfd
            fds[clientfd].fd = clientfd;
            fds[clientfd].events = POLLIN;
            maxfd = clientfd;
        } 

        // 继续遍历之后的fd
        for (int i = sockfd + 1; i <= maxfd; i ++ ) {
            // 如果fd发生读事件，说明client fd可读了
            if (fds[i].revents & POLLIN) {
                char buffer[128] = {0};
                int count = recv(i, buffer, 128, 0);
                if (count == 0) {
                    printf("disconnect\n");

                    // fd和fd对应的pollfd都清除
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    close(i);

                    continue;
                }
                send(i, buffer, count, 0);
                printf("clientfd: %d, count: %d, buffer: %s\n", i, count, buffer);
            }
        }
    }


    getchar();
    return 0;
}