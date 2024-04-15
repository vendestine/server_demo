#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


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

    // 创建一个epoll实例
    int epfd = epoll_create(1); // int size

    // 将sockfd, event封装进epoll_event变量，放入epoll实例里
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    // 定义一个events数组，用来传出io事件集合；
    struct epoll_event events[1024] = {0};

    printf("loop\n");

    while (1) {
        int nready = epoll_wait(epfd, events, 1024, -1);

        // 遍历就绪fd列表
        for (int i = 0; i < nready; i ++) {
            int connfd = events[i].data.fd;    // 取出连接的fd
            if (sockfd == connfd) {            // 如果是listen fd
                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                
                int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);

                printf("sockfd: %d\n", clientfd);

                // 将fd，event封装进epoll_evnet变量, 放入epoll实例里
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clientfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

            } 
            else if (events[i].events & EPOLLIN) {    //如果是client fd
                char buffer[10] = {0};                // buffer设小验证LT和ET模式的区别
                int count = recv(connfd, buffer, 10, 0);
                if (count == 0) {
                    printf("disconnect\n");

                    // fd, event从epoll实例里删除
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);		
                    close(i);
                    
                    continue;
                }
                send(connfd, buffer, count, 0);
                printf("clientfd: %d, count: %d, buffer: %s\n", connfd, count, buffer);
            }
        }
    }

    getchar();
    return 0;
}