#ifndef _DHASH_H
#define _DHASH_H

// 键值对类型
// 独热码：注意下面只能选一个！！！
#define KV_DHTYPE_INT_INT    0    // int key, int value
#define KV_DHTYPE_CHAR_CHAR  1    // char* key, char* value

#define ENABLE_DHASH_DEBUG    0     // 是否运行测试代码

#define DHASH_INIT_TABLE_SIZE   512   // 动态哈希表的初始长度
#define DHASH_GROW_FACTOR       2     // 动态哈希表的扩展倍数

// 定义键值对的类型
#if KV_DHTYPE_INT_INT
    typedef int  DH_KEY_TYPE;   // key类型
    typedef int  DH_VALUE_TYPE; // value类型
#elif KV_DHTYPE_CHAR_CHAR
    typedef char* DH_KEY_TYPE;    // key类型
    typedef char* DH_VALUE_TYPE;  // value类型
#endif

// 单个哈希节点定义
typedef struct dhash_node_s {
    DH_KEY_TYPE key;
    DH_VALUE_TYPE value;
} dhash_node_t;

// 哈希表结构体
typedef struct dhash_table_s {
    struct dhash_node_s** nodes; // 哈希表头
    int max_size;   // 哈希表的最大容量
    int count;      // 哈希表中存储的元素总数
} dhash_table_t;
typedef dhash_table_t kv_dhash_t;


// 初始化
// 参数：kv_dh要传地址
// 返回值：0成功，-1失败
int kv_dhash_init(kv_dhash_t* kv_dh);
// 销毁
// 参数：kv_dh要传地址
// 返回值：0成功，-1失败
int kv_dhash_desy(kv_dhash_t* kv_dh);
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_dhash_set(kv_dhash_t* kv_dh, char** tokens);
// 查找指令
// 返回值：正常返回value，NULL表示没有
char* kv_dhash_get(kv_dhash_t* kv_dh, char** tokens);
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_dhash_delete(kv_dhash_t* kv_dh, char** tokens);
// 计数指令
int kv_dhash_count(kv_dhash_t* kv_dh);
// 存在指令
// 返回值：1存在，0没有
int kv_dhash_exist(kv_dhash_t* kv_dh, char** tokens);

#endif