#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "kvstore.hpp"

#define MAX_EVENTS 1024
#define MAX_CONNS 1048576  // ① 最多支持 100w 连接

static int epfd = -1;
static ConnItem connlist[MAX_CONNS];  // ② fd 直接当下标

static int accept_cb(int fd);
static int recv_cb(int fd);
static int send_cb(int fd);

// ③ 注册/修改 epoll 事件
static void set_event(int fd, uint32_t events, bool add) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (add)
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    else
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

// listenfd 可读：有新连接
static int accept_cb(int fd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int cfd = accept(fd, (struct sockaddr*)&addr, &len);
    if (cfd < 0) return -1;

    memset(&connlist[cfd], 0, sizeof(ConnItem));
    connlist[cfd].fd = cfd;
    connlist[cfd].recv_callback = recv_cb;  // 新连接的读事件处理函数
    connlist[cfd].send_callback = send_cb;

    set_event(cfd, EPOLLIN, true);  // 先关心读
    return cfd;
}

// clientfd 可读：收到命令
static int recv_cb(int fd) {
    char* buf = connlist[fd].rbuffer;
    // ④ 留一个字节给 '\0'，防止越界
    int n = recv(fd, buf, BUFFER_LENGTH - 1, 0);
    if (n == 0) {  // ⑤ 对端正常关闭（FIN）
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        return -1;
    }
    if (n < 0) return -1;

    buf[n] = '\0';                   // 字符串结尾，strtok 要用
    kvstore_request(&connlist[fd]);  // ⑥ 协议层处理，结果在 wbuffer
    connlist[fd].wlen = strlen(connlist[fd].wbuffer);

    set_event(fd, EPOLLOUT, false);  // ⑦ 切到写事件
    return n;
}

// clientfd 可写：发响应
static int send_cb(int fd) {
    int n = send(fd, connlist[fd].wbuffer, connlist[fd].wlen, 0);
    if (n < 0) return -1;
    set_event(fd, EPOLLIN, false);  // ⑧ 发完切回读
    return n;
}

// 创建监听 socket
static int init_server(unsigned short port) {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) return -1;

    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  // ⑨ 端口复用

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;
    if (listen(sfd, 10) < 0) return -1;
    return sfd;
}

// 主循环入口（kvstore.cpp 的 main 会调它）
int epoll_entry() {
    epfd = epoll_create1(0);

    int lfd = init_server(9096);
    connlist[lfd].recv_callback = accept_cb;  // ⑩ listenfd 也用同一个数组
    set_event(lfd, EPOLLIN, true);

    printf("kvstore listening on 0.0.0.0:9096\n");

    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (events[i].events & EPOLLIN) {
                connlist[fd].recv_callback(fd);  // ⑪ 回调分发
            } else if (events[i].events & EPOLLOUT) {
                connlist[fd].send_callback(fd);
            }
        }
    }
    return 0;
}
