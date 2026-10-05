#include <arpa/inet.h>

#include <cstdio>
#include <cstring>

#include "kvstore.hpp"

extern "C" {
#include "nty_coroutine.h"
}



// 每个连接的协程函数：同步阻塞写法，但底层自动切换
void server_reader(void* arg) {
    int fd = *(int*)arg;
    int ret = 0;

    while (1) {
        ConnItem item;
        memset(&item, 0, sizeof(item));

        // recv 看起来是阻塞的，但协程库在等数据时会切走
        ret = recv(fd, item.rbuffer, BUFFER_LENGTH, 0);
        if (ret > 0) {
            kvstore_request(&item);  // 处理命令

            item.wlen = strlen(item.wbuffer);
            send(fd, item.wbuffer, item.wlen, 0);  // send 也是"阻塞"但不占 CPU
        } else if (ret == 0) {
            close(fd);
            break;
        } else {
            close(fd);
            break;
        }
    }
}

// server 协程：accept 循环
void server(void* arg) {
    unsigned short port = *(unsigned short*)arg;
    free(arg);

    int fd = nty_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr*)&local, sizeof(local));
    listen(fd, 20);
    printf("listen port: %d\n", port);

    while (1) {
        struct sockaddr_in remote;
        socklen_t len = sizeof(remote);
        int cli_fd = accept(fd, (struct sockaddr*)&remote, &len);

        // 每来一个连接，创建一个新协程处理
        nty_coroutine* read_co;
        int* pfd = new int(cli_fd);
        nty_coroutine_create(&read_co, server_reader, pfd);
    }
}

// 入口：替代 epoll_entry
int ntyco_entry(void) {
    nty_coroutine* co = nullptr;
    unsigned short* port = new unsigned short(9096);
    nty_coroutine_create(&co, server, port);  // 创建 server 协程
    nty_schedule_run();                       // 启动调度器，永不返回
    return 0;
}