// ============================================================
// testcase.cpp —— 压测客户端
// 用法：./testcase <ip> <port> [命令数]
// 例如：./testcase 127.0.0.1 9096 100000
// ============================================================
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define BUF_SIZE 512

// ① 连接服务器，返回 fd
static int connect_server(const char* ip, unsigned short port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);  // ② 字符串 IP → 网络字节序

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return -1;
    return fd;
}

// ③ 发一条命令，等一条响应（压测时不打印，省 IO）
static void do_cmd(int fd, const char* cmd) {
    send(fd, cmd, strlen(cmd), 0);
    char buf[BUF_SIZE] = {0};
    recv(fd, buf, BUF_SIZE, 0);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("usage: %s <ip> <port> [命令数]\n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    unsigned short port = (unsigned short)atoi(argv[2]);
    int count = argc > 3 ? atoi(argv[3]) : 100000;  // ④ 默认压 10w 次

    int fd = connect_server(ip, port);
    if (fd < 0) {
        printf("connect %s:%d failed\n", ip, port);
        return 1;
    }

    struct timeval tv0, tv1;
    gettimeofday(&tv0, nullptr);  // ⑤ 开始计时

    char cmd[BUF_SIZE];
    for (int i = 0; i < count; i++) {
        snprintf(cmd, BUF_SIZE, "SET Name%d King%d", i, i);
        do_cmd(fd, cmd);
        snprintf(cmd, BUF_SIZE, "GET Name%d", i);
        do_cmd(fd, cmd);
        snprintf(cmd, BUF_SIZE, "DEL Name%d", i);
        do_cmd(fd, cmd);
    }

    gettimeofday(&tv1, nullptr);  // ⑥ 结束计时
    double ms = (tv1.tv_sec - tv0.tv_sec) * 1000.0 + (tv1.tv_usec - tv0.tv_usec) / 1000.0;
    long ops = (long)count * 3;  // 每轮 SET/GET/DEL 3 条
    printf("ops=%ld, time=%.1fms, qps=%.0f\n",
           ops, ms, ms > 0 ? ops * 1000.0 / ms : 0);

    close(fd);
    return 0;
}
