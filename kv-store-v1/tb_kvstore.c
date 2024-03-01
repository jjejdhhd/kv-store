/*
 * 本代码用于测试kv存储。建立连接后的测试思路如下：
 * 1. 
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>

#include<sys/time.h>
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
#define ENABLE_QPS  0

#define max_buffer_len 1024   // 网络连接读写缓冲区长度
#define continue_test_len  100000  // 连续测试的长度

// 指令索引的枚举
typedef enum idx_engine_s{
    ENGINE_ARRAY = 0,
    ENGINE_RBTREE = 5,
    ENGINE_BTREE = 10,
    ENGINE_HASH = 15,
    ENGINE_DHASH = 20,
    ENGINE_SKIPLIST = 25,
}idx_engine;
typedef enum idx_cmd_s{
    CMD_SET = 0,
    CMD_GET,
    CMD_DELETE,
    CMD_COUNT,
    CMD_EXIST,
}idx_cmd;

// 列出发送给服务端的所有指令
// 注意上面的指令和下面对应
const char* COMMAND[] = {
    "SET" , "GET" , "DELETE" , "COUNT" , "EXIST" ,
    "RSET", "RGET", "RDELETE", "RCOUNT", "REXIST",
    "BSET", "BGET", "BDELETE", "BCOUNT", "BEXIST",
    "HSET", "HGET", "HDELETE", "HCOUNT", "HEXIST",
    "DHSET", "DHGET", "DHDELETE", "DHCOUNT", "DHEXIST",
    "ZSET", "ZGET", "ZDELETE", "ZCOUNT", "ZEXIST"
};

// 连接服务端
// 返回值：正整数表示连接的fd，-1错误
int kv_connect(const char* ip, const int port){
    int connfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in kv_addr;
    memset(&kv_addr, 0, sizeof(kv_addr));
    kv_addr.sin_family = AF_INET;
    kv_addr.sin_addr.s_addr = inet_addr(ip);
    kv_addr.sin_port = htons(port);

    int ret = connect(connfd, (struct sockaddr*)&kv_addr, sizeof(struct sockaddr_in));
    if(ret != 0){
        printf("connect error: %s\n", strerror(errno));
        return -1;
    }
    return connfd;
}

// 封装单个测试用例
// 输入参数：
// connfd：连接套接字
// cmd： 待测试命令
// res：预期返回结果
// 返回值：-1不匹配，0匹配
int kv_test_case(const int connfd, const char* cmd, const char* res){
    send(connfd, cmd, strlen(cmd), 0);

    char rbuffer[max_buffer_len] = {0};
    recv(connfd, rbuffer, max_buffer_len, 0);
    // printf("%s\n", rbuffer);
    
    if(0 == strcmp(rbuffer, res)){
        return 0;
    }else{
        printf("except_res:%s, but rbuffer:%s\n", res, rbuffer);
        return -1;
    }
}


// 所有功能的单个测试
// 返回值：0正常，负数不正常
int test_single(int connfd, idx_engine idx){
    int ret = 0;
    char s_send[32] = {0};  // 发送的指令
    snprintf(s_send, 32, "%s name humu", COMMAND[idx]);
    ret += kv_test_case(connfd, s_send, "OK\r\n");
    if(ret != 0){
        printf("\033[31mFAIL\033[0m --> send:\"%s\" in one case\n", s_send);
        return -1;
    }
    snprintf(s_send, 32, "%s name", COMMAND[idx+1]);
    ret += kv_test_case(connfd, s_send, "humu\r\n");
    ret += kv_test_case(connfd, COMMAND[idx+3], "1\r\n");
    if(ret != 0){
        printf("\033[31mFAIL\033[0m --> send:\"%s\" in one case\n", s_send);
        return -1;
    }
    snprintf(s_send, 32, "%s name", COMMAND[idx+4]);
    ret += kv_test_case(connfd, s_send, "True\r\n");
    if(ret != 0){
        printf("\033[31mFAIL\033[0m --> send:\"%s\" in one case\n", s_send);
        return -1;
    }
    snprintf(s_send, 32, "%s name", COMMAND[idx+2]);
    ret += kv_test_case(connfd, s_send, "OK\r\n");
    if(0 == ret){
        printf("\033[32mPASS\033[0m --> one case\n");  // ANSI转义序列
    }else{
        printf("\033[31mFAIL\033[0m --> send:\"%s\" in one case\n", s_send);
    }
    return ret;
}

// 所有功能的连续测试
// 返回值：0正常，负数不正常
int test_continue(int connfd, int range, idx_engine idx){
    if(connfd<=0 || range<=0){
        printf("input parameter error!\n");
        return -1;
    }
    int ret = 0;  // 返回值
    char s_send[32] = {0};  // 发送的指令
    char s_recv[32] = {0};  // 预期接收的指令
    // set、get、exist、count
    for (int i=0; i<range; i++){
        // set
        snprintf(s_send, 32, "%s %d %d", COMMAND[idx], i, i);
        ret += kv_test_case(connfd, s_send, "OK\r\n");
        if(ret != 0){
            printf("\033[31mFAIL\033[0m --> send:\"%s\" in %d continue_test\n", s_send, range);
            return -1;
        }
        // get
        snprintf(s_send, 32, "%s %d", COMMAND[idx+1], i);
        snprintf(s_recv, 32, "%d\r\n", i);
        ret += kv_test_case(connfd, s_send, s_recv);
        if(ret != 0){
            printf("\033[31mFAIL\033[0m --> send:\"%s\" in %d continue_test\n", s_send, range);
            return -1;
        }
        // exist
        snprintf(s_send, 32, "%s %d", COMMAND[idx+4], i);
        ret += kv_test_case(connfd, s_send, "True\r\n");
        if(ret != 0){
            printf("\033[31mFAIL\033[0m --> send:\"%s\" in %d continue_test\n", s_send, range);
            return -1;
        }
        // count
        snprintf(s_send, 32, "%s", COMMAND[idx+3]);
        snprintf(s_recv, 32, "%d\r\n", i+1);
        ret += kv_test_case(connfd, s_send, s_recv);
        if(ret != 0){
            printf("\033[31mFAIL\033[0m --> send:\"%s\" in %d continue_test\n", s_send, range);
            return -1;
        }
    }
    // delete
    for (int i=0; i<range; i++){
        // delete
        snprintf(s_send, 32, "%s %d", COMMAND[idx+2], i);
        ret += kv_test_case(connfd, s_send, "OK\r\n");
        if(ret != 0){
            printf("\033[31mFAIL\033[0m --> send:\"%s\" in %d continue_test\n", s_send, range);
            return -1;
        }
    }
    // count
    snprintf(s_send, 32, "%s", COMMAND[idx+3]);
    ret += kv_test_case(connfd, s_send, "0\r\n");
    if(0 == ret){
        printf("\033[32mPASS\033[0m --> %d continue_test\n", range);
    }else{
        printf("\033[31mFAIL\033[0m --> send:\"%s\" in final continue_test\n", s_send);
    }
    return ret;
}

// 计算set/get/delete指令的耗时，其他指令无需测试qps
int test_cmd_timeused(int connfd, int range, idx_engine i_engine, idx_cmd i_cmd){
    // 定义时间结构体
    struct timeval tv_begin;
    struct timeval tv_end;
    // 测试QPS
    char s_send[32] = {0};  // 发送的指令
    char s_recv[32] = {0};  // 发送的指令
    int ret = 0;
    if(i_cmd == CMD_SET){
        gettimeofday(&tv_begin, NULL);
        for(int i=0; i<range; i++){
            snprintf(s_send, 32, "%s %d %d", COMMAND[i_engine+i_cmd], i, i);
            ret += kv_test_case(connfd, s_send, "OK\r\n");
        }
        gettimeofday(&tv_end, NULL);
    }else if(i_cmd == CMD_GET){
        gettimeofday(&tv_begin, NULL);
        for(int i=0; i<range; i++){
            snprintf(s_send, 32, "%s %d", COMMAND[i_engine+i_cmd], i);
            snprintf(s_recv, 32, "%d\r\n", i);
            ret += kv_test_case(connfd, s_send, s_recv);
        }
        gettimeofday(&tv_end, NULL);
    }else if(i_cmd == CMD_DELETE){
        gettimeofday(&tv_begin, NULL);
        for(int i=0; i<range; i++){
            snprintf(s_send, 32, "%s %d", COMMAND[i_engine+i_cmd], i);
            ret += kv_test_case(connfd, s_send, "OK\r\n");
        }
        gettimeofday(&tv_end, NULL);
    }
    if(ret == 0){
        int time_ms = TIME_SUB_MS(tv_end, tv_begin);
        return time_ms;
    }else{
        printf("recv unexpected message from server...\n");
        return 0;
    }
}

// 根据set/get/delete指令耗时计算QPS
float test_cmd_qps(int connfd, int range, idx_engine i_engine, idx_cmd i_cmd){
    // 判断指令是否符合预期
    if(i_cmd!=CMD_SET && i_cmd!=CMD_GET && i_cmd!=CMD_DELETE){
        printf("i_cmd = GET or SET or DELETE!\n");
        return 0.0;
    }
    // 开始计算
    int time_ms = test_cmd_timeused(connfd, range, i_engine, i_cmd);
    float qps = (float)range / (float)time_ms * 1000;
    printf("total %ss:%d  time_used:%d(ms)  qps:%.2f(%ss/sec)\n",
            COMMAND[i_engine+i_cmd], range, time_ms, qps, COMMAND[i_engine+i_cmd]);
    return qps;
}

// ./tb_kvstore ip port
int main(int argc, char* argv[]){
    // 解析命令行参数并连接
    if(argc != 3){
        printf("error! argv format: ./tb_kvstore ip port\n");
        return -1;
    }
    const char* ip = argv[1];
    const int port = atoi(argv[2]);
    int connfd = kv_connect(ip, port);
    if(connfd <= 0) return -1;

    // array测试
    printf("---->test array\n");
    if(test_single(connfd, ENGINE_ARRAY) != 0) return -1;
    if(test_continue(connfd, 40, ENGINE_ARRAY) != 0) return -1;
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_ARRAY, CMD_SET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_ARRAY, CMD_GET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_ARRAY, CMD_DELETE);


    // rbtree测试
    printf("---->test rbtree\n");
    if(test_single(connfd, ENGINE_RBTREE) != 0) return -1;
    if(test_continue(connfd, 40, ENGINE_RBTREE) != 0) return -1;
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_RBTREE, CMD_SET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_RBTREE, CMD_GET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_RBTREE, CMD_DELETE);

    // btree测试
    printf("---->test btree\n");
    if(test_single(connfd, ENGINE_BTREE) != 0) return -1;
    if(test_continue(connfd, 40, ENGINE_BTREE) != 0) return -1;
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_BTREE, CMD_SET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_BTREE, CMD_GET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_BTREE, CMD_DELETE);

    // hash测试
    printf("---->test hash\n");
    if(test_single(connfd, ENGINE_HASH) != 0) return -1;
    if(test_continue(connfd, 40, ENGINE_HASH) != 0) return -1;
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_HASH, CMD_SET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_HASH, CMD_GET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_HASH, CMD_DELETE);

    // dhash测试
    printf("---->test dhash\n");
    if(test_single(connfd, ENGINE_DHASH) != 0) return -1;
    if(test_continue(connfd, 40, ENGINE_DHASH) != 0) return -1;
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_DHASH, CMD_SET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_DHASH, CMD_GET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_DHASH, CMD_DELETE);
    
    // skiplist测试
    printf("---->test skiplist\n");
    if(test_single(connfd, ENGINE_SKIPLIST) != 0) return -1;
    if(test_continue(connfd, 40, ENGINE_SKIPLIST) != 0) return -1;
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_SKIPLIST, CMD_SET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_SKIPLIST, CMD_GET);
    if(ENABLE_QPS) test_cmd_qps(connfd, continue_test_len, ENGINE_SKIPLIST, CMD_DELETE);
    
    close(connfd);
    return 0;
}