#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"dhash.h"


/*-----------------------------函数声明------------------------------*/
// 计算哈希值
// 输入参数：
//    key：键
//    size：哈希表大小
// 返回值：正数为哈希值，-1为错误
static int dhash_function(DH_KEY_TYPE key, int size);
// 创建哈希节点
// 返回值：NULL失败，非空创建成功
dhash_node_t* dhash_node_create(DH_KEY_TYPE key, DH_KEY_TYPE value);
// 销毁哈希节点
// 返回值：-1失败，0成功
int dhash_node_desy(dhash_node_t* node);
// 初始化哈希表
// 输入参数：
//     hash：哈希结构体的地址
// 返回值：0成功，-1失败
int dhash_table_init(dhash_table_t* dhash, int size);
// 销毁哈希表
// 返回值：-1失败，0成功
int dhash_table_desy(dhash_table_t* dhash);
// 插入元素：有冲突则顺延到第一个空节点
// 返回值：0成功，-1失败，-2已经有相应的key
int dhash_node_insert(dhash_table_t *dhash, DH_KEY_TYPE key, DH_KEY_TYPE value);
// 查找元素
// 返回值：非负数表示索引，-1表示没找到
int dhash_node_search(dhash_table_t* dhash, DH_KEY_TYPE key);
// 删除元素
// 返回值：0成功，-1失败，-2没有
int dhash_node_delete(dhash_table_t* dhash, DH_KEY_TYPE key);
// 打印哈希表
// 返回值：0成功，-1失败
int dhash_table_print(dhash_table_t* dhash);
/*------------------------------------------------------------------*/


/*-----------------------------函数定义------------------------------*/
// 计算哈希值
// 输入参数：
//    key：键
//    size：哈希表大小
// 返回值：正数为哈希值，-1为错误
static int dhash_function(DH_KEY_TYPE key, int size){
#if KV_DHTYPE_INT_INT
    // 直接对键的大小取余
    if(key < 0) return -1;
    return key % size;
#elif KV_DHTYPE_CHAR_CHAR
    unsigned long int sum = 0;
    for (int i=0; i<strlen(key); i++) {
        sum = sum*37 + key[i];
    }
    sum = sum % size;
    return (int) sum;
#endif
}

// 创建哈希节点
// 返回值：NULL失败，非空创建成功
dhash_node_t* dhash_node_create(DH_KEY_TYPE key, DH_KEY_TYPE value){
    dhash_node_t *node = (dhash_node_t*)calloc(1, sizeof(dhash_node_t));
    if (!node) return NULL;
#if KV_DHTYPE_INT_INT
    node->key = key;
    node->value = value;
#elif KV_DHTYPE_CHAR_CHAR
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
    return node;
}

// 销毁哈希节点
// 返回值：-1失败，0成功
int dhash_node_desy(dhash_node_t* node){
    if(node == NULL) return -1;
    if(node->value){
        free(node->value);
        node->value = NULL;
    }
    if(node->key){
        free(node->key);
        node->key = NULL;
    }
    free(node);
    node = NULL;
    return 0;
}

// 初始化哈希表
// 输入参数：
//     hash：哈希结构体的地址
//     size：哈希表的大小
// 返回值：0成功，-1失败
int dhash_table_init(dhash_table_t* dhash, int size){
    if (dhash == NULL) return -1;
    dhash->nodes = (dhash_node_t**)calloc(size, sizeof(dhash_node_t*));
    if(dhash->nodes == NULL) return -1;
    dhash->max_size = size;
    dhash->count = 0;
    return 0;
}

// 销毁哈希表
// 返回值：-1失败，0成功
int dhash_table_desy(dhash_table_t* dhash){
    if (dhash == NULL) return -1;
    for (int i=0; i<dhash->max_size; i++) {
        // 删除单个节点上的全部链表
        while(dhash->nodes[i] != NULL){ // error
        int ret = dhash_node_desy(dhash->nodes[i]);
            dhash->count--;
            dhash->nodes[i] = NULL;
            if(ret != 0) return -1;
        }
    }
    if(dhash->nodes){
        free(dhash->nodes);
        dhash->nodes = NULL;
    };
    dhash->max_size = 0;
    dhash->count = 0;
    return 0;
}

// 插入元素：有冲突则顺延到第一个空节点
// 返回值：0成功，-1失败，-2已经有相应的key
int dhash_node_insert(dhash_table_t *dhash, DH_KEY_TYPE key, DH_KEY_TYPE value){
#if KV_HTYPE_INT_INT
    if (!dhash || key<0) return -1;
#elif KV_HTYPE_CHAR_CHAR
    if (!dhash || !key || !value) return -1;
#endif
    // 首先看看是否需要扩展哈希表
    if(dhash->count > (dhash->max_size>>1)){
        dhash_table_t new_table;
        int ret = dhash_table_init(&new_table, dhash->max_size*DHASH_GROW_FACTOR);
        if(ret != 0) return -1;
        // 搬移元素
        for(int i=0; i<dhash->max_size; i++){
            if(dhash->nodes[i] != NULL){
                ret = dhash_node_insert(&new_table, dhash->nodes[i]->key, dhash->nodes[i]->value);
                if(ret != 0) return ret;
            }
        }
        // 交换表头
        dhash->max_size *= DHASH_GROW_FACTOR;
        dhash->count = new_table.count;
        dhash_node_t** tmp_nodes = dhash->nodes;
        dhash->nodes = new_table.nodes;
        new_table.nodes = tmp_nodes;
        new_table.max_size /= DHASH_GROW_FACTOR;
        ret = dhash_table_desy(&new_table);
        if(ret != 0) return ret;
    }
    // 找到要插入的空节点
    int idx = dhash_function(key, dhash->max_size);
    while(dhash->nodes[idx] != NULL){
    #if KV_DHTYPE_INT_INT
        if(dhash->nodes[idx]->key != key)
    #elif KV_DHTYPE_CHAR_CHAR
        if(dhash->nodes[idx]->key!=NULL && strcmp(dhash->nodes[idx]->key, key)!=0)
    #endif
        {
            if(idx == dhash->max_size-1){
                idx = 0;
            }else{
                idx++;
            }
        }
    #if KV_DHTYPE_INT_INT
        else if(dhash->nodes[idx]->key == key)
    #elif KV_DHTYPE_CHAR_CHAR
        else if(dhash->nodes[idx]->key!=NULL && strcmp(dhash->nodes[idx]->key, key)==0)
    #endif
        {
            return -2;
        }
    }
    // 创建新的节点加入
    dhash->nodes[idx] = dhash_node_create(key, value);
    if(dhash->nodes[idx] == NULL){
        return -1;
    }
    dhash->count++;
    return 0;
}

// 查找元素：从起始位置遍历所有节点查找
// 返回值：非负数表示索引，-1表示没找到
int dhash_node_search(dhash_table_t* dhash, DH_KEY_TYPE key){
    int idx = dhash_function(key, dhash->max_size);
    for(int i=0; i<dhash->max_size; i++){
        if(dhash->nodes[idx] != NULL){
        #if KV_DHTYPE_INT_INT
            if(dhash->nodes[idx]->key == key)
        #elif KV_DHTYPE_CHAR_CHAR
            if(strcmp(dhash->nodes[idx]->key, key) == 0)
        #endif
            {
                break;
            }
        }
        // idx = (idx+i) % dhash->max_size;  // 下面的写法更快
        if(idx == dhash->max_size-1){
            idx = 0;
        }else{
            idx++;
        }
    }
#if KV_DHTYPE_INT_INT
    if(dhash->nodes[idx]==NULL || dhash->nodes[idx]->key != key)
#elif KV_DHTYPE_CHAR_CHAR
    if(dhash->nodes[idx]==NULL || (strcmp(dhash->nodes[idx]->key, key)!=0))
#endif
    {
        return -1;
    }else{
        return idx;
    }
}

// 删除元素
// 返回值：0成功，-1失败，-2没有
int dhash_node_delete(dhash_table_t* dhash, DH_KEY_TYPE key){
    // 首先看看是否需要缩减哈希表
    // 存储元素小于1/4空间，按照“增长因子DHASH_GROW_FACTOR”缩减
    if((dhash->count < (dhash->max_size>>4)) && (dhash->max_size>DHASH_INIT_TABLE_SIZE)){
        dhash_table_t new_table;
        int ret = dhash_table_init(&new_table, dhash->max_size/DHASH_GROW_FACTOR);
        if(ret != 0) return -1;
        // 搬移元素
        for(int i=0; i<dhash->max_size; i++){
            if(dhash->nodes[i] != NULL){
                ret = dhash_node_insert(&new_table, dhash->nodes[i]->key, dhash->nodes[i]->value);
                if(ret != 0) return ret;
            }
        }
        // 交换表头
        dhash->max_size /= DHASH_GROW_FACTOR;
        dhash->count = new_table.count;
        dhash_node_t** tmp_nodes = dhash->nodes;
        dhash->nodes = new_table.nodes;
        new_table.nodes = tmp_nodes;
        new_table.max_size *= DHASH_GROW_FACTOR;
        ret = dhash_table_desy(&new_table);
        if(ret != 0) return ret;
    }
    // 查找元素
    int idx = dhash_node_search(dhash, key);
    if(idx < 0){
        return -2;
    }else{
        int ret = dhash_node_desy(dhash->nodes[idx]);
        dhash->nodes[idx] = NULL;
        dhash->count--;
        return ret;
    }
}

// 打印哈希表
// 返回值：0成功，-1失败
int dhash_table_print(dhash_table_t* dhash){
    if(dhash==NULL) return -1;
    for(int i=0; i<dhash->max_size; i++){
        dhash_node_t* cur_node = dhash->nodes[i];
        if(cur_node != NULL){
            printf("idx %d:", i);
            #if KV_DHTYPE_INT_INT
                printf(" key=%d", cur_node->key);
            #elif KV_DHTYPE_CHAR_CHAR
                printf(" key=%s", cur_node->key);
            #endif
            printf("\n");
        }
    }
    return 0;
}
/*------------------------------------------------------------------*/


/*----------------------------kv存储协议-----------------------------*/
// 初始化
// 参数：kv_dh要传地址
// 返回值：0成功，-1失败
int kv_dhash_init(kv_dhash_t* kv_dh){
    if(kv_dh == NULL) return -1;
    return dhash_table_init(kv_dh, DHASH_INIT_TABLE_SIZE);
}
// 销毁
// 参数：kv_dh要传地址
// 返回值：0成功，-1失败
int kv_dhash_desy(kv_dhash_t* kv_dh){
    if(kv_dh == NULL) return -1;
    return dhash_table_desy(kv_dh);
}
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_dhash_set(kv_dhash_t* kv_dh, char** tokens){
    if(kv_dh==NULL || tokens==NULL || tokens[1]==NULL || tokens[2]==NULL) return -1;
    return dhash_node_insert(kv_dh, tokens[1], tokens[2]);
}
// 查找指令
// 返回值：正常返回value，NULL表示没有
char* kv_dhash_get(kv_dhash_t* kv_dh, char** tokens){
    if(kv_dh==NULL || tokens==NULL || tokens[1]==NULL) return NULL;
    int idx = dhash_node_search(kv_dh, tokens[1]);
    if(idx >= 0){
        return kv_dh->nodes[idx]->value;
    }else{
        return NULL;
    }
}
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_dhash_delete(kv_dhash_t* kv_dh, char** tokens){
    return dhash_node_delete(kv_dh, tokens[1]);
}
// 计数指令
int kv_dhash_count(kv_dhash_t* kv_dh){
    return kv_dh->count; 
}
// 存在指令
// 返回值：1存在，0没有
int kv_dhash_exist(kv_dhash_t* kv_dh, char** tokens){
    return (dhash_node_search(kv_dh, tokens[1]) >= 0);
}
/*------------------------------------------------------------------*/


/*-----------------------------测试代码------------------------------*/
#if ENABLE_DHASH_DEBUG
#if KV_DHTYPE_INT_INT
int main(){
    return 0;
}
#elif KV_DHTYPE_CHAR_CHAR
int main(){
    return 0;
}
#endif
#endif
/*------------------------------------------------------------------*/
