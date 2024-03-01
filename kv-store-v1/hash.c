#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"hash.h"

/*-----------------------------函数声明------------------------------*/
// 计算哈希值
// 输入参数：
//    key：键
//    size：哈希表大小
// 返回值：正数为哈希值，-1为错误
static int _hash(H_KEY_TYPE key, int size);
// 创建哈希节点
// 返回值：NULL失败，非空创建成功
hashnode_t* hash_node_create(H_KEY_TYPE key, H_VALUE_TYPE value);
// 销毁哈希节点
// 返回值：-1失败，0成功
int hash_node_desy(hashnode_t* node);
// 初始化哈希表
// 输入参数：
//     hash：哈希结构体的地址
// 返回值：0成功，-1失败
int hash_table_init(hashtable_t* hash);
// 销毁哈希表
// 返回值：-1失败，0成功
int hash_table_desy(hashtable_t* hash);
// 插入元素：有冲突则是头插法
// 返回值：0成功，-1失败，-2已经有相应的key
int hash_node_insert(hashtable_t *hash, H_KEY_TYPE key, H_VALUE_TYPE value);
// 查找元素
// 返回值：非空成功，NULL没找到
hashnode_t* hash_node_search(hashtable_t* hash, H_KEY_TYPE key);
// 删除元素
// 返回值：0成功，-1失败，-2没有
int hash_node_delete(hashtable_t* hash, H_KEY_TYPE key);
// 打印哈希表
// 返回值：0成功，-1失败
int hash_table_print(hashtable_t* hash);
/*------------------------------------------------------------------*/


/*-----------------------------函数定义------------------------------*/
// 计算哈希值
// 输入参数：
//    key：键
//    size：哈希表大小
// 返回值：正数为哈希值，-1为错误
static int _hash(H_KEY_TYPE key, int size) {
#if KV_HTYPE_INT_INT
    // 直接对键的大小取余
    if (key < 0) return -1;
    return key % size;
#elif KV_HTYPE_CHAR_CHAR
    // 所有字符的ASCII码累加后取余
    if (key == NULL) return -1;
    int sum = 0;
    for(int i=0; key[i]!=0; i++) {
        sum += key[i];  // 空字符的ASCII码为0
    }
    return sum % size;
#endif
}

// 创建哈希节点
// 返回值：NULL失败，非空创建成功
hashnode_t* hash_node_create(H_KEY_TYPE key, H_VALUE_TYPE value) {
    hashnode_t *node = (hashnode_t*)calloc(1, sizeof(hashnode_t));
    if (!node) return NULL;
#if KV_HTYPE_INT_INT
    node->key = key;
    node->value = value;
#elif KV_HTYPE_CHAR_CHAR
    char* kcopy = (char*)calloc(strlen(key)+1, sizeof(char));
    if(kcopy == NULL){
        free(node);
        node = NULL;
        return NULL;
    }
    char* vcopy = (char*)calloc(strlen(value)+1, sizeof(char));
    if(vcopy == NULL){
        free(kcopy);
        kcopy = NULL;
        free(node);
        node = NULL;
        return NULL;
    }
    strncpy(kcopy, key, strlen(key)+1);
    strncpy(vcopy, value, strlen(value)+1);
    node->key = kcopy;
    node->value = vcopy;
#endif
    node->next = NULL;
    return node;
}

// 销毁哈希节点
// 返回值：-1失败，0成功
int hash_node_desy(hashnode_t* node){
    if(node == NULL) return -1;
    if(node->value){
        free(node->value);
        node->value = NULL;
    }
    if(node->key){
        free(node->key);
        node->key = NULL;
    }
    node->next = NULL;
    free(node);
    node = NULL;
    return 0;
}

// 初始化哈希表
// 输入参数：
//     hash：哈希结构体的地址
// 返回值：0成功，-1失败
int hash_table_init(hashtable_t* hash){
    if (hash == NULL) return -1;
    hash->nodes = (hashnode_t**)calloc(HASH_TABLE_SIZE, sizeof(hashnode_t*));
    if(hash->nodes == NULL) return -1;
    hash->table_size = HASH_TABLE_SIZE;
    hash->count = 0;
    return 0;
}

// 销毁哈希表
// 返回值：-1失败，0成功
int hash_table_desy(hashtable_t* hash){
    if (hash == NULL) return -1;
    for (int i=0; i<hash->table_size; i++) {
        // 删除单个节点上的全部链表
        hashnode_t* node = hash->nodes[i];
        while(node != NULL){ // error
            hashnode_t* tmp = node;
            node = node->next;
            hash->nodes[i] = node;
            hash->count--;
            if(hash_node_desy(tmp) != 0) return -1;
        }
    }
    if(hash->nodes){
        free(hash->nodes);
        hash->nodes = NULL;
    };
    hash->table_size = 0;
    hash->count = 0;
    return 0;
}

// 插入元素：有冲突则是头插法
// 返回值：0成功，-1失败，-2已经有相应的key
int hash_node_insert(hashtable_t *hash, H_KEY_TYPE key, H_VALUE_TYPE value){
#if KV_HTYPE_INT_INT
    if (!hash || key<0) return -1;
#elif KV_HTYPE_CHAR_CHAR
    if (!hash || !key || !value) return -1;
#endif
    // 找到要插入的位置
    int idx = _hash(key, hash->table_size);
    hashnode_t *node = hash->nodes[idx];
    while (node != NULL) {
    #if KV_HTYPE_INT_INT
        if(node->key == key) return -2;
    #elif KV_HTYPE_CHAR_CHAR
        if(strcmp(node->key, key) == 0) return -2;
    #endif
        node = node->next;
    }
    // 创建新的节点加入
    hashnode_t* new_node = hash_node_create(key, value);
    if(new_node == NULL) return -1;
    new_node->next = hash->nodes[idx];
    hash->nodes[idx] = new_node;
    hash->count++;
    return 0;
}

// 查找元素
// 返回值：非空成功返回节点，NULL没找到
hashnode_t* hash_node_search(hashtable_t* hash, H_KEY_TYPE key){
#if KV_HTYPE_INT_INT
    if (!hash || key<0) return NULL;
#elif KV_HTYPE_CHAR_CHAR
    if (!hash || !key) return NULL;
#endif
    int idx = _hash(key, hash->table_size);
    hashnode_t* node = hash->nodes[idx];
    while(node != NULL){
    #if KV_HTYPE_INT_INT
        if(node->key == key) return node;
    #elif KV_HTYPE_CHAR_CHAR
        if(strcmp(node->key, key) == 0) return node;
    #endif
        node = node->next;
    }
    return NULL;
}

// 删除元素
// 返回值：0成功，-1失败，-2没有
int hash_node_delete(hashtable_t* hash, H_KEY_TYPE key){
#if KV_HTYPE_INT_INT
    if (!hash || key<0) return -1;
#elif KV_HTYPE_CHAR_CHAR
    if (!hash || !key) return -1;
#endif

    int idx = _hash(key, hash->table_size);
    hashnode_t* cur_node = hash->nodes[idx];
    if (cur_node == NULL) return -2;  // 没找到
    // 首先判断第一个元素
#if KV_HTYPE_INT_INT
    if (cur_node->key == key)
#elif KV_HTYPE_CHAR_CHAR
    if (strcmp(cur_node->key, key) == 0)
#endif
    {
        hashnode_t* next_node = cur_node->next;
        hash->nodes[idx] = next_node;
        if(hash_node_desy(cur_node) == 0){
            hash->count --;
            return 0;
        }else{
            return -1;
        }
    }
    // 第一个元素不是就遍历剩下的链表
    else {
        hashnode_t* last_node = hash->nodes[idx];
        cur_node = last_node->next;
        while (cur_node != NULL) {
        #if KV_HTYPE_INT_INT
            if (cur_node->key == key) break;
        #elif KV_HTYPE_CHAR_CHAR
            if (strcmp(cur_node->key, key) == 0) break;
        #endif
            last_node = cur_node;
            cur_node = cur_node->next;
        }
        if(cur_node == NULL){
            return -2;
        }else{
            last_node->next = cur_node->next;
            hash->count--;
            return hash_node_desy(cur_node);
        }
    }
}

// 打印哈希表
// 返回值：0成功，-1失败
int hash_table_print(hashtable_t* hash){
    if(hash==NULL) return -1;
    for(int i=0; i<hash->table_size; i++){
        hashnode_t* cur_node = hash->nodes[i];
        if(cur_node != NULL){
            printf("idx %d:", i);
            while(cur_node != NULL){
            #if KV_HTYPE_INT_INT
                printf(" %d", cur_node->key);
            #elif KV_HTYPE_CHAR_CHAR
                printf(" %s", cur_node->key);
            #endif
                cur_node = cur_node->next;
            }
            printf("\n");
        }
    }
    return 0;
}
/*------------------------------------------------------------------*/


/*----------------------------kv存储协议-----------------------------*/
// 初始化
// 参数：kv_h要传地址
// 返回值：0成功，-1失败
int kv_hash_init(kv_hash_t* kv_h){
    if(kv_h == NULL) return -1;
    return hash_table_init(kv_h);
}
// 销毁
// 参数：kv_h要传地址
// 返回值：0成功，-1失败
int kv_hash_desy(kv_hash_t* kv_h){
    if(kv_h == NULL) return -1;
    return hash_table_desy(kv_h);
}
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_hash_set(kv_hash_t* kv_h, char** tokens){
    if(kv_h==NULL || tokens==NULL || tokens[1]==NULL || tokens[2]==NULL) return -1;
    return hash_node_insert(kv_h, tokens[1], tokens[2]);
}
// 查找指令
// 返回值：正常返回value，NULL表示没有
char* kv_hash_get(kv_hash_t* kv_h, char** tokens){
    if(kv_h==NULL || tokens==NULL || tokens[1]==NULL) return NULL;
    hashnode_t* node = hash_node_search(kv_h, tokens[1]);
    if(node == NULL){
        return NULL;
    }else{
        return node->value;
    }
}
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_hash_delete(kv_hash_t* kv_h, char** tokens){
    return hash_node_delete(kv_h, tokens[1]);
}
// 计数指令
int kv_hash_count(kv_hash_t* kv_h){
    return kv_h->count;
}
// 存在指令
// 返回值：1存在，0没有
int kv_hash_exist(kv_hash_t* kv_h, char** tokens){
    return (hash_node_search(kv_h, tokens[1]) != NULL);
}
/*------------------------------------------------------------------*/


/*-----------------------------测试代码------------------------------*/
#if HASH_H_DEBUG
#if KV_HTYPE_INT_INT
int main(){
    return 0;
}
#elif KV_HTYPE_CHAR_CHAR
int main(){
    return 0;
}
#endif
#endif
/*------------------------------------------------------------------*/
