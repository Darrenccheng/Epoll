#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
    // 1. 创建用于通信的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket");
        exit(0);
    }

    // 2. 连接服务器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;     // ipv4
    addr.sin_port = htons(9999);   // 服务器监听的端口, 字节序应该是网络字节序
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(ret == -1)
    {
        perror("connect");
        exit(0);
    }

    // 通信
    while(1)
    {
        // 读数据
        char recvBuf[1024];
        // 写数据
        // sprintf(recvBuf, "data: %d\n", i++);
        if (fgets(recvBuf, sizeof(recvBuf), stdin) == NULL) {
            break;
        }
        // 去掉末尾的换行符
        size_t len = strlen(recvBuf);
        if (len > 0 && recvBuf[len - 1] == '\n') {
            recvBuf[len - 1] = '\0';
        }

        write(fd, recvBuf, strlen(recvBuf));
        // 如果客户端没有发送数据, 默认阻塞
        memset(recvBuf, 0, sizeof(recvBuf));
        read(fd, recvBuf, sizeof(recvBuf) - 1); // 为什么在服务端逐个字符发来的时候，也能全部接收服务端返回的数据？
        printf("recv buf: %s\n", recvBuf);
        sleep(1);
    }

    // 释放资源
    close(fd);

    return 0;
}