/*
 * zv开头的变量是zvnet异步网络库（epoll）。
 * kv开头的变量是kv存储协议解析。
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<errno.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<fcntl.h>
#include<sys/epoll.h>

#include"kvstore.h"

/*-------------------------------------------------------*/
/*-----------------------异步网路库-----------------------*/
/*-------------------------------------------------------*/
/*-----------------------函数声明-------------------------*/
#define max_buffer_len      1024    // 读写buffer长度
#define epoll_events_size   1024    // epoll就绪集合大小
#define connblock_size      1024    // 单个连接块存储的连接数量
#define listen_port_count   1       // 监听端口总数

// 有如下参数列表和返回之类型的函数都称为 CALLBACK
// 回调函数，方便在特定epoll事件发生时执行相应的操作
typedef int (*ZV_CALLBACK)(int fd, int events, void *arg);
// 回调函数：建立连接
int accept_cb(int fd, int event, void* arg);
// 回调函数：接收数据
int recv_cb(int clientfd, int event, void* arg);
// 回调函数：发送数据
int send_cb(int clientfd, int event, void* arg);

// 单个连接
typedef struct zv_connect_s{
    // 本连接的客户端fd
    int fd;
    // 本连接的读写buffer
    char rbuffer[max_buffer_len];
    size_t rcount;  // 读buffer的实际大小
    char wbuffer[max_buffer_len];
    size_t wcount;  // 写buffer的实际大小
    size_t next_len;  // 下一次读数据长度（读取多个包会用到）
    // 本连接的回调函数--accept()/recv()/send()
    ZV_CALLBACK cb;
}zv_connect;

// 连接块的头
typedef struct zv_connblock_s{
    struct zv_connect_s *block;  // 指向的当前块，注意大小为 connblock_size
    struct zv_connblock_s *next;  // 指向的下一个连接块的头
}zv_connblock;

// 反应堆结构体
typedef struct zv_reactor_s{
    int epfd;   // epoll文件描述符
    // struct epoll_event events[epoll_events_size];  // 就绪事件集合
    struct zv_connblock_s *blockheader;  // 连接块的第一个头
    int blkcnt;  // 现有的连接块的总数
}zv_reactor;

// reactor初始化
int init_reactor(zv_reactor *reactor);
// reator销毁
void destory_reactor(zv_reactor* reactor);
// 服务端初始化：将端口设置为listen状态
int init_sever(int port);
// 将本地的listenfd添加进epoll
int set_listener(zv_reactor *reactor, int listenfd, ZV_CALLBACK cb);
// 创建一个新的连接块（尾插法）
int zv_create_connblock(zv_reactor* reactor);
// 根据fd从连接块中找到连接所在的位置
// 逻辑：整除找到所在的连接块、取余找到在连接块的位置
zv_connect* zv_connect_idx(zv_reactor* reactor, int fd);
// 运行kv存储协议
int kv_run_while(int argc, char *argv[]);
/*-------------------------------------------------------*/


/*-----------------------函数定义-------------------------*/
// reactor初始化
int init_reactor(zv_reactor *reactor){
    if(reactor == NULL) return -1;
    // 初始化参数
    memset(reactor, 0, sizeof(zv_reactor));
    reactor->epfd = epoll_create(1);
    if(reactor->epfd <= 0){
        printf("init reactor->epfd error: %s\n", strerror(errno));
        return -1;
    }
    // 为链表集合分配内存
    reactor->blockheader = (zv_connblock*)calloc(1, sizeof(zv_connblock));
    if(reactor->blockheader == NULL) return -1;
    reactor->blockheader->next = NULL;
    // 为链表集合中的第一个块分配内存
    reactor->blockheader->block = (zv_connect*)calloc(connblock_size, sizeof(zv_connect));
    if(reactor->blockheader->block == NULL) return -1;
    reactor->blkcnt = 1;
    return 0;
}

// reator销毁
void destory_reactor(zv_reactor* reactor){
    if(reactor){
        close(reactor->epfd);  // 关闭epoll
        zv_connblock* curblk = reactor->blockheader;
        zv_connblock* nextblk = reactor->blockheader;
        do{
            curblk = nextblk;
            nextblk = curblk->next;
            if(curblk->block) free(curblk->block);
            if(curblk) free(curblk);
        }while(nextblk != NULL);
    }
}

// 服务端初始化：将端口设置为listen状态
int init_sever(int port){
    // 创建服务端
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // io
    // fcntl(sockfd, F_SETFL, O_NONBLOCK);  // 非阻塞
    // 设置网络地址和端口
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;  // IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 0.0.0.0，任何地址都可以连接本服务器
    servaddr.sin_port = htons(port);  // 端口
    // 将套接字绑定到一个具体的本地地址和端口
    if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))){
        printf("bind failed: %s", strerror(errno));
        return -1;
    }
    // 将端口设置为listen（并不会阻塞程序执行）
    listen(sockfd, 10);  // 等待连接队列的最大长度为10
    printf("listen port: %d, sockfd: %d\n", port, sockfd);
    return sockfd;
}

// 将本地的listenfd添加进epoll
int set_listener(zv_reactor *reactor, int listenfd, ZV_CALLBACK cb){
    if(!reactor || !reactor->blockheader) return -1;
    // 将服务端放进连接块
    reactor->blockheader->block[listenfd].fd = listenfd;
    reactor->blockheader->block[listenfd].cb = cb;  // listenfd的回调函数应该是accept()
    // 将服务端添加进epoll事件
    struct epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;
    epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, listenfd, &ev);
    return 0;
}

// 创建一个新的连接块（尾插法）
int zv_create_connblock(zv_reactor* reactor){
    if(!reactor) return -1;
    // 初始化新的连接块
    zv_connblock* newblk = (zv_connblock*)calloc(1, sizeof(zv_connblock));
    if(newblk == NULL) return -1;
    newblk->block = (zv_connect*)calloc(connblock_size, sizeof(zv_connect));
    if(newblk->block == NULL) return -1;
    newblk->next = NULL;
    // 找到最后一个连接块
    zv_connblock* endblk = reactor->blockheader;
    while(endblk->next != NULL){
        endblk = endblk->next;
    }
    // 添加上新的连接块
    endblk->next = newblk;
    reactor->blkcnt++;
    return 0;
}

// 根据fd从连接块中找到连接所在的位置
// 逻辑：整除找到所在的连接块、取余找到在连接块的位置
zv_connect* zv_connect_idx(zv_reactor* reactor, int fd){
    if(!reactor) return NULL;
    // 计算fd应该在的连接块
    int blkidx = fd / connblock_size;
    while(blkidx >= reactor->blkcnt){
        zv_create_connblock(reactor);
        // printf("create a new connblk!\n");
    }
    // 找到这个连接块
    zv_connblock* blk = reactor->blockheader;
    int i = 0;
    while(++i < blkidx){
        blk = blk->next;
    }
    return &blk->block[fd % connblock_size];
}

// 回调函数：建立连接
// fd：服务端监听端口listenfd
// event：没用到，但是回调函数的常用格式
// arg：应该是reactor*
int accept_cb(int fd, int event, void* arg){
    // 与客户端建立连接
    struct sockaddr_in clientaddr;  // 连接到本服务器的客户端信息
    socklen_t len_sockaddr = sizeof(clientaddr);
    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len_sockaddr);
    if(clientfd < 0){
        printf("accept() error: %s\n", strerror(errno));
        return -1;
    }
    // 将连接添加进连接块
    zv_reactor* reactor = (zv_reactor*)arg;
    zv_connect* conn = zv_connect_idx(reactor, clientfd);
    conn->fd = clientfd;
    conn->cb = recv_cb;
    conn->next_len = max_buffer_len;
    conn->rcount = 0;
    conn->wcount = 0;
    // 将客户端添加进epoll事件
    struct epoll_event ev;
    ev.data.fd = clientfd;
    ev.events = EPOLLIN;  // 默认水平触发（有数据就触发）
    epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, clientfd, &ev);
    printf("connect success! sockfd:%d, clientfd:%d\n", fd, clientfd);
}

// 回调函数：接收数据
int recv_cb(int clientfd, int event, void* arg){
    zv_reactor* reactor = (zv_reactor*)arg;
    zv_connect* conn = zv_connect_idx(reactor, clientfd);

    int recv_len = recv(clientfd, conn->rbuffer+conn->rcount, conn->next_len, 0);  // 由于当前fd可读所以没有阻塞
    if(recv_len < 0){
        printf("recv() error: %s\n", strerror(errno));
        close(clientfd);
        // return -1;
        exit(0);
    }else if(recv_len == 0){
        // 重置对应的连接块
        conn->fd = -1;
        conn->rcount = 0;
        conn->wcount = 0;
        // 从epoll监听事件中移除
        epoll_ctl(reactor->epfd, EPOLL_CTL_DEL, clientfd, NULL);
        // 关闭连接
        close(clientfd);
        printf("close clientfd:%d\n", clientfd);
    }else if(recv_len > 0){
        conn->rcount += recv_len;
        // conn->next_len = *(short*)conn->rbuffer;  // 从tcp协议头中获取数据长度，假设前两位是长度
        
        // 处理接收到的字符串，并将需要发回的信息存储在缓冲区中
        // printf("recv clientfd:%d, len:%d, mess: %s\n", clientfd, recv_len, conn->rbuffer);
        conn->rcount = kv_protocal(conn->rbuffer, max_buffer_len);

        // 将kv存储的回复消息（rbuffer）拷贝给wbuffer
        // printf("msg:%s len:%ld\n", msg, strlen(msg));
        memset(conn->wbuffer, '\0', max_buffer_len);
        memcpy(conn->wbuffer, conn->rbuffer, conn->rcount);
        conn->wcount = conn->rcount;
        memset(conn->rbuffer, 0, max_buffer_len);
        conn->rcount = 0;

        // 将本连接更改为epoll写事件
        conn->cb = send_cb;
        struct epoll_event ev;
        ev.data.fd = clientfd;
        ev.events = EPOLLOUT;
        epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, clientfd, &ev);
    }
    return 0;
}

// 回调函数：发送数据
int send_cb(int clientfd, int event, void* arg){
    zv_reactor* reactor = (zv_reactor*)arg;
    zv_connect* conn = zv_connect_idx(reactor, clientfd);

    int send_len = send(clientfd, conn->wbuffer, conn->wcount, 0);
     if(send_len < 0){
        printf("send() error: %s\n", strerror(errno));
        close(clientfd);
        return -1;
     }
    memset(conn->wbuffer, 0, conn->next_len);
    conn->wcount -= send_len;

    // 发送完成后将本连接再次更改为读事件
    conn->cb = recv_cb;
    struct epoll_event ev;
    ev.data.fd = clientfd;
    ev.events = EPOLLIN;
    epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, clientfd, &ev);
    return 0;
}


// 运行kv存储协议
int kv_run_while(int argc, char *argv[]){
    // 创建管理连接的反应堆
    // zv_reactor reactor;
    zv_reactor *reactor = (zv_reactor*)malloc(sizeof(zv_reactor));
    init_reactor(reactor);
    // 服务端初始化
    int start_port = atoi(argv[1]);
    for(int i=0; i<listen_port_count; i++){
        int sockfd = init_sever(start_port+i);
        set_listener(reactor, sockfd, accept_cb);  // 将sockfd添加进epoll
    }
    printf("init finish, listening connet...\n");
    // 开始监听事件
    struct epoll_event events[epoll_events_size] = {0};  // 就绪事件集合
    while(1){
        // 等待事件发生
        int nready = epoll_wait(reactor->epfd, events, epoll_events_size, -1);  // -1等待/0不等待/正整数则为等待时长
        if(nready == -1){
            printf("epoll_wait error: %s\n", strerror(errno));
            break;
        }else if(nready == 0){
            continue;
        }else if(nready > 0){
            // printf("process %d epoll events...\n", nready);
            // 处理所有的就绪事件
            int i = 0;
            for(i=0; i<nready; i++){
                int connfd = events[i].data.fd;
                zv_connect* conn = zv_connect_idx(reactor, connfd);

                // 回调函数和下面的的逻辑实现了数据回环
                if(EPOLLIN & events[i].events){
                    conn->cb(connfd, events[i].events, reactor);
                }
                if(EPOLLOUT & events[i].events){
                    conn->cb(connfd, events[i].events, reactor);
                } 
            }
        }
    }
    destory_reactor(reactor);
    return 0;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("please enter port! e.x. 9999.\n");
        return -1;
    }
    // 初始化存储引擎
    kv_engine_init();

    // 运行kv存储
    kv_run_while(argc, argv);
    
    // 销毁存储引擎
    kv_engine_desy();
    return 0;
}
/*-------------------------------------------------------*/
