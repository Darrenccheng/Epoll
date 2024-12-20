#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>

struct socketinfo {
    int fd;
    int epfd;
};

void acceptConn(std::unique_ptr<socketinfo> info) {
    printf("子线程 id: %d\n", std::this_thread::get_id());
    int cfd = accept(info->fd, NULL, NULL);
    // 边缘出发下，设置文件描述符的属性为非阻塞
    int flag =fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFD, flag);

    // 新得到的文件描述符添加到epoll模型中, 下一轮循环的时候就可以被检测了
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;    // 读缓冲区是否有数据
    ev.data.fd = cfd;
    int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
    if(ret == -1)
    {
        perror("epoll_ctl-accept");
        //delete info;
        exit(0);
    }

    //delete info;
    return;
}

void commuication(std::unique_ptr<socketinfo> info) {
    char buf[4];
    memset(buf, 0, sizeof(buf));
    while (1) {
        int len = recv(info->fd, buf, sizeof(buf) - 1, 0);
        if(len == 0)
        {
            printf("客户端已经断开了连接\n");
            // 将这个文件描述符从epoll模型中删除
            epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
            close(info->fd);
            break;
        }
        else if(len > 0)
        {
            buf[len] = '\0';
            printf("客户端say: %s\n", buf);
            send(info->fd, buf, len, 0);
        }
        else
        {
            if (errno == EAGAIN) {
                printf("数据以及读完了。。。\n");
                break;
            }
            else {
                perror("recv");
                break;
            }

        }
    }
    //delete info;
    return;
}

// server
int main(int argc, const char* argv[])
{
    // 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1)
    {
        perror("socket error");
        exit(1);
    }

    // 绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9999);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 本地多有的ＩＰ

    // 设置端口复用
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定端口
    int ret = bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(ret == -1)
    {
        perror("bind error");
        exit(1);
    }

    // 监听
    ret = listen(lfd, 64);
    if(ret == -1)
    {
        perror("listen error");
        exit(1);
    }

    // 现在只有监听的文件描述符
    // 所有的文件描述符对应读写缓冲区状态都是委托内核进行检测的epoll
    // 创建一个epoll模型
    int epfd = epoll_create(100);
    if(epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }

    // 往epoll实例中添加需要检测的节点, 现在只有监听的文件描述符
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;    // 检测lfd读读缓冲区是否有数据
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if(ret == -1)
    {
        perror("epoll_ctl");
        exit(0);
    }

    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(struct epoll_event);
    // 持续检测
    while(1)
    {
        // 调用一次, 检测一次
        int num = epoll_wait(epfd, evs, size, -1);

        for(int i=0; i<num; ++i)
        {
            // 取出当前的文件描述符
            int curfd = evs[i].data.fd;
            std::unique_ptr<socketinfo> info(new socketinfo);
            info->fd = curfd;
            info->epfd = epfd;
            // 判断这个文件描述符是不是用于监听的
            if(curfd == lfd)
            {
                // 建立新的连接
               std::thread t(acceptConn, std::move(info));
               t.detach();
            }
            else
            {
                // 处理通信的文件描述符
                // 接收数据
                std::thread t(commuication, std::move(info));
                t.detach();
            }
        }
    }
    close(lfd);
    return 0;
}