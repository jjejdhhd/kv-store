#ifndef _ARRAY_H
#define _ARRAY_H

#define kv_array_block_size 32  // 单个块存储的最大元素数量

// 单个键值对结构体
typedef struct kv_array_item_s{
    char* key;
    char* value;
}kv_array_item_t;

// 存储整个元素的块
typedef struct kv_array_block_s{
    struct kv_array_block_s* next;
    kv_array_item_t* items;  // 数组大小为 kv_array_block_size
    int count;  // 当前块存储的kv数量，最大为kv_array_block_size
}kv_array_block_t;

// array结构类型头
typedef struct kv_array_s{
    struct kv_array_block_s* head;
    int count;  // 当前存储的键值对数量
}kv_array_t;


// 初始化
// 参数：kv_a要传地址
// 返回值：0成功，-1失败
int kv_array_init(kv_array_t* kv_a);
// 销毁
// 参数：kv_a要传地址
// 返回值：0成功，-1失败
int kv_array_desy(kv_array_t* kv_a);
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_array_set(kv_array_t* kv_a, char** tokens);
// 查找指令
// 返回值：非空表示有，NULL表示没有
char* kv_array_get(kv_array_t* kv_a, char** tokens);
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_array_delete(kv_array_t* kv_a, char** tokens);
// 计数指令
int kv_array_count(kv_array_t* kv_a);
// 存在指令
// 返回值：0存在，-1没有
int kv_array_exist(kv_array_t* kv_a, char** tokens);

#endif
