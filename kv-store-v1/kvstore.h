#ifndef _KVSTORE_H
#define _KVSTORE_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"array.h"
#include"rbtree.h"
#include"btree.h"
#include"hash.h"
#include"dhash.h"
#include"skiplist.h"

#define max_tokens          32      // 用户指令最大的拆分数量

// 存储引擎
kv_array_t  kv_a;
kv_rbtree_t kv_rb;
kv_btree_t  kv_b;
kv_hash_t  kv_h;
kv_dhash_t  kv_dh;
kv_skiplist_t  kv_z;

// 初始化kv存储引擎
// 返回值：0成功，-1失败
int kv_engine_init(void);
// 销毁kv存储引擎
// 返回值：0成功，-1失败
int kv_engine_desy(void);
// 实现完整的kv存储协议
// 输入参数：
//    buffer：字符缓冲区的地址
//    max_buffer_len：字符缓冲区的最大长度
// 返回值：表示需要发送给客户端的字符串长度
size_t kv_protocal(char* buffer, size_t max_buffer_len);

#endif