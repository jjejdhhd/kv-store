#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"kvstore.h"

/*-------------------------------------------------------*/
/*-----------------------kv存储协议-----------------------*/
/*-------------------------------------------------------*/

// 列出kv存储协议中所有的指令
typedef enum kv_cmd_e{
    KV_CMD_START = 0,   // 指令开始
    // array
    KV_CMD_SET = KV_CMD_START,  // 插入
    KV_CMD_GET,         // 查找
    KV_CMD_DELETE,      // 删除
    KV_CMD_COUNT,       // 计数
    KV_CMD_EXIST,       // 存在
    // rbtree
    KV_CMD_RSET,
    KV_CMD_RGET,
    KV_CMD_RDELETE,
    KV_CMD_RCOUNT,
    KV_CMD_REXIST,
    // btree
    KV_CMD_BSET,
    KV_CMD_BGET,
    KV_CMD_BDELETE,
    KV_CMD_BCOUNT,
    KV_CMD_BEXIST,
    // hash
    KV_CMD_HSET,
    KV_CMD_HGET,
    KV_CMD_HDELETE,
    KV_CMD_HCOUNT,
    KV_CMD_HEXIST,
    // dhash
    KV_CMD_DHSET,
    KV_CMD_DHGET,
    KV_CMD_DHDELETE,
    KV_CMD_DHCOUNT,
    KV_CMD_DHEXIST,
    // skiplist
    KV_CMD_ZSET,
    KV_CMD_ZGET,
    KV_CMD_ZDELETE,
    KV_CMD_ZCOUNT,
    KV_CMD_ZEXIST,
    // cmd
    KV_CMD_ERROR,   // 指令格式错误
    KV_CMD_QUIT,    // 
    KV_CMD_END,     // 指令结束
}kv_cmd;


// 用户指令结构体
typedef struct kv_user_argc_s{
    const char* cmd;  // 当前指令
    const int argc;  // 当前指令对应的参数数量
}kv_user_argc;

// 列出用户可以使用的所有指令
// 注意这个数组的存储顺序要和上面的枚举类型保持一致
const kv_user_argc KV_COMMAND[] = {
    {"SET" ,2}, {"GET" ,1}, {"DELETE" ,1}, {"COUNT" ,0}, {"EXIST" ,1},
    {"RSET",2}, {"RGET",1}, {"RDELETE",1}, {"RCOUNT",0}, {"REXIST",1},
    {"BSET",2}, {"BGET",1}, {"BDELETE",1}, {"BCOUNT",0}, {"BEXIST",1},
    {"HSET",2}, {"HGET",1}, {"HDELETE",1}, {"HCOUNT",0}, {"HEXIST",1},
    {"DHSET",2}, {"DHGET",1}, {"DHDELETE",1}, {"DHCOUNT",0}, {"DHEXIST",1},
    {"ZSET",2}, {"ZGET",1}, {"ZDELETE",1}, {"ZCOUNT",0}, {"ZEXIST",1}
};

// 给客户端返回信息的枚举
typedef enum zv_res_t{
    // set、delete
    KV_RES_OK = 0,
    KV_RES_AL_HAVE,
    KV_RES_FAIL,
    // get、delete
    KV_RES_NO_KEY,
    // exist
    KV_RES_TRUE,
    KV_RES_FALSE,
    // other
    KV_RES_ERROR,
}zv_res;

// 列出具体的返回信息
const char* RES_MSG[] = {
    "OK\r\n", "Already have this key!\r\n", "Fail\r\n",
    "No such key!\r\n",
    "True\r\n", "False\r\n",
    "Error command!\r\n"
};

// 初始化kv存储引擎
// 返回值：0成功，-1失败
int kv_engine_init(void){
    int ret = 0;
    ret += kv_array_init(&kv_a);
    ret += kv_rbtree_init(&kv_rb);
    ret += kv_btree_init(&kv_b, 6);
    ret += kv_hash_init(&kv_h);
    ret += kv_dhash_init(&kv_dh);
    ret += kv_skiplist_init(&kv_z, 6);
    return ret;
}
// 销毁kv存储引擎
// 返回值：0成功，-1失败
int kv_engine_desy(void){
    int ret = 0;
    ret += kv_array_desy(&kv_a);
    ret += kv_rbtree_desy(&kv_rb);
    ret += kv_btree_desy(&kv_b);
    ret += kv_hash_desy(&kv_h);
    ret += kv_dhash_desy(&kv_dh);
    ret += kv_skiplist_desy(&kv_z);
    return ret;
}


// 按照空格切分用户指令，返回切割的数量
int kv_split_tokens(char** tokens, char* msg){
    int count = 0;  // 解析的指令长度
    char* ptr;
    tokens[count] = strtok_r(msg, " ", &ptr);
    // while((token != NULL) && (count<max_token)){
    while(tokens[count] != NULL){
        count++;
        tokens[count] = strtok_r(NULL, " ", &ptr);
    };
    return count--;
}

// 解析用户指令，判断用户输入的是哪一个kv_cmd
int kv_parser_cmd(char** tokens, int num_tokens){
    if(tokens==NULL || tokens[0]==NULL || num_tokens==0){
        return KV_CMD_ERROR;
    }
    int idx = 0;
    for(idx=KV_CMD_START; idx<KV_CMD_ERROR; idx++){
        if(strcmp(tokens[0], KV_COMMAND[idx].cmd) == 0){
            if(KV_COMMAND[idx].argc == num_tokens-1){
                break;
            } else{
                // printf("\"%s\" should follow %d argv...\n", tokens[0], KV_COMMAND[idx].argc);
                return KV_CMD_ERROR;
            }
        }
    }
    return idx;
    // 若都不匹配会返回KV_CMD_ZEXIST的下一个，也就是KV_CMD_ERROR
}

// set指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_set(char* buffer, int ret){
    size_t msg_len = 0;  // 返回缓冲区的有效字符串长度
    // 成功
    if(ret == 0){
        msg_len = strlen(RES_MSG[KV_RES_OK]);
        strncpy(buffer, RES_MSG[KV_RES_OK], msg_len);
    }
    // 已经有相应的key
    else if(ret == -2){
        msg_len = strlen(RES_MSG[KV_RES_AL_HAVE]);
        strncpy(buffer, RES_MSG[KV_RES_AL_HAVE], msg_len);
    }
    // 一般的失败
    else{
        msg_len = strlen(RES_MSG[KV_RES_FAIL]);
        strncpy(buffer, RES_MSG[KV_RES_FAIL], msg_len);
    }
    return msg_len;
}

// get指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_get(char* buffer, size_t max_buffer_len, char* value){
    size_t msg_len = 0;  // 返回缓冲区的有效字符串长度
    if(value == NULL){
        msg_len = strlen(RES_MSG[KV_RES_NO_KEY]);
        strncpy(buffer, RES_MSG[KV_RES_NO_KEY], msg_len);
    }else{
        // msg_len = strlen(kv_a.items[idx].value) + 2;
        // strncpy(buffer, kv_a.items[idx].value, msg_len);
        msg_len = snprintf(buffer, max_buffer_len, "%s\r\n", value);
    }
    return msg_len;
}

// delete指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_delete(char* buffer, int ret){
    size_t msg_len = 0;  // 返回缓冲区的有效字符串长度
    if(ret == -2){
        msg_len = strlen(RES_MSG[KV_RES_NO_KEY]);
        strncpy(buffer, RES_MSG[KV_RES_NO_KEY], msg_len);
    }else if(ret == -1){
        msg_len = strlen(RES_MSG[KV_RES_FAIL]);
        strncpy(buffer, RES_MSG[KV_RES_FAIL], msg_len);
    }else{
        msg_len = strlen(RES_MSG[KV_RES_OK]);
        strncpy(buffer, RES_MSG[KV_RES_OK], msg_len);
    }
    return msg_len;
}

// count指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_count(char* buffer, int cnt){
    size_t msg_len = 0;  // 返回缓冲区的有效字符串长度
    char s_cnt[10];
    msg_len = snprintf(s_cnt, sizeof(s_cnt), "%d\r\n", cnt);
    strncpy(buffer, s_cnt, msg_len);
    return msg_len;
}

// exist指令返回的信息拷贝到缓冲区
size_t kv_setbuffer_exist(char* buffer, int ret){
    size_t msg_len = 0;  // 返回缓冲区的有效字符串长度
    if(ret == 1){
        msg_len = strlen(RES_MSG[KV_RES_TRUE]);
        strncpy(buffer, RES_MSG[KV_RES_TRUE], msg_len);
    }else{
        msg_len = strlen(RES_MSG[KV_RES_FALSE]);
        strncpy(buffer, RES_MSG[KV_RES_FALSE], msg_len);
    }
    return msg_len;
}

// 实现完整的kv存储协议
// 输入参数：
//    buffer：字符缓冲区的地址
//    max_buffer_len：字符缓冲区的最大长度
// 返回值：表示需要发送给客户端的字符串长度
size_t kv_protocal(char* buffer, size_t max_buffer_len){
    // 分割
    char* tokens[max_tokens] = {NULL};
    int num_tokens = kv_split_tokens(tokens, buffer);
    // // 打印解析的指令
    // for (int i=0; i<num_tokens; i++){
    //     printf("%s\n", tokens[i]);
    // }

    // 解析并执行指令
    int user_cmd = kv_parser_cmd(tokens, num_tokens);
    size_t msg_len = 0;  // 返回缓冲区的有效字符串长度
    switch(user_cmd){
        // array
        case KV_CMD_SET:{
            int ret = kv_array_set(&kv_a, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        case KV_CMD_GET:{
            char* value = kv_array_get(&kv_a, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        case KV_CMD_DELETE:{
            int ret = kv_array_delete(&kv_a, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }
        case KV_CMD_COUNT:{
            int cnt = kv_array_count(&kv_a);
            msg_len = kv_setbuffer_count(buffer, cnt);
            break;
        }
        case KV_CMD_EXIST:{
            int ret = kv_array_exist(&kv_a, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        // rbtree
        case KV_CMD_RSET:{
            int ret = kv_rbtree_set(&kv_rb, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        case KV_CMD_RGET:{
            char* value = kv_rbtree_get(&kv_rb, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        case KV_CMD_RDELETE:{
            int ret = kv_rbtree_delete(&kv_rb, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }
        case KV_CMD_RCOUNT:{
            int cnt = kv_rbtree_count(&kv_rb);
            msg_len = kv_setbuffer_count(buffer, cnt);
            break;
        }
        case KV_CMD_REXIST:{
            int ret = kv_rbtree_exist(&kv_rb, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        // Btree
        case KV_CMD_BSET:{
            int ret = kv_btree_set(&kv_b, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        case KV_CMD_BGET:{
            char* value = kv_btree_get(&kv_b, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        case KV_CMD_BDELETE:{
            int ret = kv_btree_delete(&kv_b, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }
        case KV_CMD_BCOUNT:{
            int cnt = kv_btree_count(&kv_b);
            msg_len = kv_setbuffer_count(buffer, cnt);
            break;
        }
        case KV_CMD_BEXIST:{
            int ret = kv_btree_exist(&kv_b, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        // hash
        case KV_CMD_HSET:{
            int ret = kv_hash_set(&kv_h, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        case KV_CMD_HGET:{
            char* value = kv_hash_get(&kv_h, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        case KV_CMD_HDELETE:{
            int ret = kv_hash_delete(&kv_h, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }
        case KV_CMD_HCOUNT:{
            int cnt = kv_hash_count(&kv_h);
            msg_len = kv_setbuffer_count(buffer, cnt);
            break;
        }
        case KV_CMD_HEXIST:{
            int ret = kv_hash_exist(&kv_h, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        // dhash
        case KV_CMD_DHSET:{
            int ret = kv_dhash_set(&kv_dh, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        case KV_CMD_DHGET:{
            char* value = kv_dhash_get(&kv_dh, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        case KV_CMD_DHDELETE:{
            int ret = kv_dhash_delete(&kv_dh, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }
        case KV_CMD_DHCOUNT:{
            int cnt = kv_dhash_count(&kv_dh);
            msg_len = kv_setbuffer_count(buffer, cnt);
            break;
        }
        case KV_CMD_DHEXIST:{
            int ret = kv_dhash_exist(&kv_dh, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        // skiplist
        case KV_CMD_ZSET:{
            int ret = kv_skiplist_set(&kv_z, tokens);
            msg_len = kv_setbuffer_set(buffer, ret);
            break;
        }
        case KV_CMD_ZGET:{
            char* value = kv_skiplist_get(&kv_z, tokens);
            msg_len = kv_setbuffer_get(buffer, max_buffer_len, value);
            break;
        }
        case KV_CMD_ZDELETE:{
            int ret = kv_skiplist_delete(&kv_z, tokens);
            msg_len = kv_setbuffer_delete(buffer, ret);
            break;
        }
        case KV_CMD_ZCOUNT:{
            int cnt = kv_skiplist_count(&kv_z);
            msg_len = kv_setbuffer_count(buffer, cnt);
            break;
        }
        case KV_CMD_ZEXIST:{
            int ret = kv_skiplist_exist(&kv_z, tokens);
            msg_len = kv_setbuffer_exist(buffer, ret);
            break;
        }
        // others
        case KV_CMD_ERROR:{
            msg_len = strlen(RES_MSG[KV_RES_ERROR]);
            strncpy(buffer, RES_MSG[KV_RES_ERROR], msg_len);
            break;
        }
        default:{
            msg_len = strlen(RES_MSG[KV_RES_ERROR]);
            strncpy(buffer, RES_MSG[KV_RES_ERROR], msg_len);
        }
    }
    return msg_len;
}

