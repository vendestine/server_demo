#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>


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

    // 初始化可读集合
	fd_set rfds, rset;
	FD_ZERO(&rfds);

    // 将listen fd放入可读集合中，更新maxfd；
	FD_SET(sockfd, &rfds);
	int maxfd = sockfd; 

	printf("loop\n");

    while (1) {
		rset = rfds;      // 原始的集合用来设置，拷贝一份可读集合，用于内核判断 	
		int nready = select(maxfd+1, &rset, NULL, NULL, NULL); 

        // 如果listen fd已就绪
		if (FD_ISSET(sockfd, &rset)) {
			struct sockaddr_in clientaddr;
			socklen_t len = sizeof(clientaddr);
			
			int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

			printf("sockfd: %d\n", clientfd);

            // 将client fd放入集合，更新maxfd
			FD_SET(clientfd, &rfds);
			maxfd = clientfd;
		}

        // 继续遍历socfd之后的fd
		for (int i = sockfd + 1; i <= maxfd; i ++) {
            // 如果有fd已就绪，说明是client fd
			if (FD_ISSET(i, &rset)) {
				char buffer[128] = {0};
				int count = recv(i, buffer, 128, 0);
				if (count == 0) {
					printf("disconnect\n");

                    // fd和fd_set里的fd对应位都清除
					FD_CLR(i, &rfds);
					close(i);
					
					break;
				}
				send(i, buffer, count, 0);
				printf("clientfd: %d, count: %d, buffer: %s\n", i, count, buffer);
			}
		}
	}

    getchar();
    return 0;
}