#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define BUFFER_LENGTH		1024

typedef int (*RCALLBACK)(int fd);

// 读事件对应的回调函数
int accept_cb(int fd);
int recv_cb(int fd);
// 写事件对应的回调函数
int send_cb(int fd);

// conn, fd, buffer, callback
struct conn_item {
	int fd;
	// buffer
	char rbuffer[BUFFER_LENGTH];
	int rlen;
	char wbuffer[BUFFER_LENGTH];
	int wlen;

    // callback函数指针
	union {
		RCALLBACK accept_callback;
		RCALLBACK recv_callback;
	} recv_t;
	RCALLBACK send_callback;
};


int epfd = 0;   //epoll fd定义在全局，方便在所有函数里使用
struct conn_item connlist[1024] = {0};

// 设置io上监听的事件   封装epoll的ctl操作
int set_event(int fd, int event, int flag) {
	if (flag) { // 1 add, 0 mod
		struct epoll_event ev;
		ev.events = event ;
		ev.data.fd = fd;
		epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	} else {
		struct epoll_event ev;
		ev.events = event;
		ev.data.fd = fd;
		epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
	}
}

// 触发读事件 调用的accept回调函数
int accept_cb(int fd) {
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	
	int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
	if (clientfd < 0) {
		return -1;
	}

    // 触发事件，做一次操作，继续设置读事件
	set_event(clientfd, EPOLLIN, 1);

    // clientfd封装到conn_item
	connlist[clientfd].fd = clientfd;
	memset(connlist[clientfd].rbuffer, 0, BUFFER_LENGTH);
	connlist[clientfd].rlen = 0;
	memset(connlist[clientfd].wbuffer, 0, BUFFER_LENGTH);
	connlist[clientfd].wlen = 0;	
	connlist[clientfd].recv_t.recv_callback = recv_cb;
	connlist[clientfd].send_callback = send_cb;

	return clientfd;
}

// 触发读事件 调用的recv回调函数
int recv_cb(int fd) {
    // 拿到现在的read buffer和idx
	char *buffer = connlist[fd].rbuffer;
	int idx = connlist[fd].rlen;
	
    // 收到的数据从buffer + idx处开始拼接，返回这次收到的数量
	int count = recv(fd, buffer+idx, BUFFER_LENGTH-idx, 0);
	connlist[fd].rlen += count;
	if (count == 0) {
		printf("disconnect\n");

		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);		
		close(fd);
		
		return -1;
	}

    // 关注是否可读，读完后关注是否可写 => 触发事件，做一次操作，设置成写事件，触发再调用send_cb
	set_event(fd, EPOLLOUT, 0);

#if 1 // 回声业务，把read buffer的数据直接拷贝到write buffer回发
	memcpy(connlist[fd].wbuffer, connlist[fd].rbuffer, connlist[fd].rlen);
	connlist[fd].wlen = connlist[fd].rlen;
#endif

	return count;
}


// 调用send的回调函数
int send_cb(int fd) {
    // 拿到现在的write buffer和idx
	char *buffer = connlist[fd].wbuffer;
	int idx = connlist[fd].wlen;

    // 发送当前buffer里所有的数据
	int count = send(fd, buffer, idx, 0);

    // 关注io是否可写，写完后关注io是否可读，=> 触发事件，做一次操作，设置成写事件，触发再调用send_cb
	set_event(fd, EPOLLIN, 0);

	return count;
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


    // listen fd封装到conn_item
	connlist[sockfd].fd = sockfd;
	connlist[sockfd].recv_t.accept_callback = accept_cb;

    // 创建一个epoll实例
	epfd = epoll_create(1);
	set_event(sockfd, EPOLLIN, 1);  //设置listen fd上监听的事件
	struct epoll_event events[1024] = {0};
	
	while (1) { // mainloop();
		int nready = epoll_wait(epfd, events, 1024, -1); 

		for (int i = 0; i < nready; i ++) {
			int connfd = events[i].data.fd;
            // 如果是读事件，触发相关回调函数
			if (events[i].events & EPOLLIN) {
				int count = connlist[connfd].recv_t.recv_callback(connfd);
				printf("recv count: %d recv <-- rbuffer: %s\n", count, connlist[connfd].rbuffer);
			} 
            // 如果是写事件，触发相关回调函数
            else if (events[i].events & EPOLLOUT) { 
				printf("send --> wbuffer: %s\n",  connlist[connfd].wbuffer);
				int count = connlist[connfd].send_callback(connfd);
			}
		}
	}

	getchar();

}




