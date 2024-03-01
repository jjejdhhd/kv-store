#ifndef __SKIPLIST_H
#define __SKIPLIST_H

// 键值对类型
// 独热码：注意下面只能选一个！！！
#define KV_ZTYPE_INT_INT    0    // int key, int value
#define KV_ZTYPE_CHAR_CHAR  1    // char* key, char* value

#define ENABLE_SKIPLIST_DEBUG    0     // 是否运行测试代码

// 定义键值对的类型
#if KV_ZTYPE_INT_INT
    typedef int  Z_KEY_TYPE;   // key类型
    typedef int  Z_VALUE_TYPE; // value类型
#elif KV_ZTYPE_CHAR_CHAR
    typedef char* Z_KEY_TYPE;    // key类型
    typedef char* Z_VALUE_TYPE;  // value类型
#endif

// 跳表单节点的结构体
typedef struct skiplist_node_s{
    Z_KEY_TYPE key;
    Z_VALUE_TYPE value;
    struct skiplist_node_s** next;  // 每一层指向后面的节点
}skiplist_node_t;

// 跳表结构体
typedef struct skiplist_s{
    struct skiplist_node_s* header;  // 跳表的头节点
    int count;  // 总的元素数量
    int cur_level;  // 跳表的当前总层级
    int max_level;  // 跳表的最大层级，初始化完成后就不会再改变
}skiplist_t;
typedef skiplist_t kv_skiplist_t;

// 初始化
// 参数：kv_z要传地址
// 返回值：0成功，-1失败
int kv_skiplist_init(kv_skiplist_t* kv_z, int m);
// 销毁
// 参数：kv_z要传地址
// 返回值：0成功，-1失败
int kv_skiplist_desy(kv_skiplist_t* kv_z);
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_skiplist_set(kv_skiplist_t* kv_z, char** tokens);
// 查找指令
// 返回值：正常返回value，NULL表示没有
char* kv_skiplist_get(kv_skiplist_t* kv_z, char** tokens);
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_skiplist_delete(kv_skiplist_t* kv_z, char** tokens);
// 计数指令
int kv_skiplist_count(kv_skiplist_t* kv_z);
// 存在指令
// 返回值：1存在，0没有
int kv_skiplist_exist(kv_skiplist_t* kv_z, char** tokens);

#endif