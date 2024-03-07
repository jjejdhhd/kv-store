#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#define max_recv_len 1024  // 缓冲区的最大长度

int main(int argc, char* argv[]) {
    // 根据命令行参数读取“端口”
    if(argc != 2){
        printf("please enter port! e.x. 9999.\n");
        return -1;
    }
    int listen_port = atoi(argv[1]);

    // 创建服务端的套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // io
    // 设置网络地址和端口
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;  // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0，任何地址都可以连接本服务器
    servaddr.sin_port = htons(listen_port);  // 端口
    // 将套接字绑定到一个具体的本地地址和端口
    if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))){
        printf("bind failed: %s", strerror(errno));
        return -1;
    }
    // 将端口设置为listen（并不会阻塞程序执行）
    listen(sockfd, 10);  // 等待连接队列的最大长度为10
    printf("sockfd=%d, port:%d listening\n", sockfd, listen_port);

    // 等待与客户端连接
    struct sockaddr_in clientaddr;  // 连接到本服务器的客户端信息
    socklen_t len_sockaddr = sizeof(struct sockaddr);
    int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len_sockaddr);  // 阻塞等待
    printf("connect success! sockfd:%d, clientfd:%d\n", sockfd, clientfd);

    // echo：将客户端发送过来的信息原封不动发回去
    char buffer[max_recv_len] = {0};
    while(1){
        memset(buffer, 0, max_recv_len);  // 清空缓存区
        int recv_len = recv(clientfd, buffer, max_recv_len, 0);  // 接收数据，阻塞等待
        if(0 == recv_len){
            close(clientfd);  // recv()返回0表明断开连接
            break;
        }
        printf("recv clientfd:%d, recv_len:%d, recv: %s\n", clientfd, recv_len, buffer);
        send(clientfd, buffer, max_recv_len, 0);  // 发送数据
    }
    return 0;
}
