#include<stdio.h>
#include<stdlib.h>
// #include<time.h>
#include<string.h>
#include"skiplist.h"

/*-----------------------------函数声明------------------------------*/
// 创建节点并初始化
// 返回值：非空成功，NULL失败
skiplist_node_t* skiplist_node_create(Z_KEY_TYPE key, Z_VALUE_TYPE value, int level);
// 销毁节点
// 返回值：0成功，-1失败
int skiplist_node_desy(skiplist_node_t* node);
// 跳表初始化
// 输入参数：
//    list：kv_h要传地址
//    max_level：跳表的最大层级
// 返回值：0成功，-1失败
int skiplist_init(skiplist_t* list, int max_level);
// 销毁跳表：
// 返回值：0成功，-1失败
int skiplist_desy(skiplist_t *list);
// 插入元素：有冲突则是头插法
// 返回值：0成功，-1失败，-2已经有相应的key
int skiplist_node_insert(skiplist_t* list, Z_KEY_TYPE key, Z_VALUE_TYPE value);
// 查找元素
// 返回值：非空成功返回节点，NULL没找到
skiplist_node_t* skiplist_node_search(skiplist_t* list, Z_KEY_TYPE key);
// 删除元素
// 返回值：0成功，-1失败，-2没有
int skiplist_node_delete(skiplist_t* list, Z_KEY_TYPE key);
// 打印整个跳表
// 返回值：0成功，-1失败
int skiplist_print(skiplist_t* list);
/*------------------------------------------------------------------*/


/*-----------------------------函数定义------------------------------*/
// 创建一个层级为level的节点
// 返回值：非空成功，NULL失败
skiplist_node_t* skiplist_node_create(Z_KEY_TYPE key, Z_VALUE_TYPE value, int level){
#if KV_ZTYPE_INT_INT
    if(key<=0 || level<=0) return NULL;
#elif KV_ZTYPE_CHAR_CHAR
    if(key==NULL || value==NULL || level<=0) return NULL;
#endif
    // 创建节点
    skiplist_node_t* new = (skiplist_node_t*)calloc(1, sizeof(skiplist_node_t));
    if (new == NULL) return NULL;
    // 初始化next
    new->next = (skiplist_node_t**)calloc(level, sizeof(skiplist_node_t*));
    if (new->next == NULL) {
        free(new);
        new = NULL;
        return NULL;
    }
    for (int i=0; i<level; i++) {
        new->next[i] = NULL;
    }
    // 初始化键值对
#if KV_ZTYPE_INT_INT
    new->key = key;
    new->value = value;
#elif KV_ZTYPE_CHAR_CHAR
    char* kcopy = (char*)calloc(strlen(key)+1, sizeof(char));
    if(kcopy == NULL){
        skiplist_node_desy(new);
        return NULL;
    }
    char* vcopy = (char*)calloc(strlen(value)+1, sizeof(char));
    if(vcopy == NULL){
        free(kcopy);
        kcopy = NULL;
        skiplist_node_desy(new);
        return NULL;
    }
    strncpy(kcopy, key, strlen(key)+1);
    strncpy(vcopy, value, strlen(value)+1);
    new->key = kcopy;
    new->value = vcopy;
#endif
    return new;
}

// 销毁节点
// 返回值：0成功，-1失败
int skiplist_node_desy(skiplist_node_t* node){
    if(node == NULL) return -1;
#if KV_ZTYPE_INT_INT
    node->value = 0;
    node->key = 0;
#elif KV_ZTYPE_CHAR_CHAR
    if(node->value){
        free(node->value);
        node->value = NULL;
    }
    if(node->key){
        free(node->key);
        node->key = NULL;
    }
#endif
    if(node->next){
        free(node->next);
        node->next = NULL;
    }
    free(node);
    node = NULL;
    return 0;
}

// 跳表初始化
// 输入参数：
//    list：kv_h要传地址
//    max_level：跳表的最大层级
// 返回值：0成功，-1失败
int skiplist_init(skiplist_t* list, int max_level){
    if(list==NULL || max_level<=0) return -1;
    // 创建空节点作为头节点
    list->header = (skiplist_node_t*)calloc(1, sizeof(skiplist_node_t));
    if (list->header == NULL) return -1;
    // 初始化头节点的next
    list->header->next = (skiplist_node_t**)calloc(max_level, sizeof(skiplist_node_t*));
    if (list->header->next == NULL) {
        free(list->header);
        list->header = NULL;
        return -1;
    }
    for (int i=0; i<max_level; i++) {
        list->header->next[i] = NULL;
    }
#if KV_ZTYPE_INT_INT
    list->header->key = key;
    list->header->value = value;
#elif KV_ZTYPE_CHAR_CHAR
    list->header->key = NULL;
    list->header->value = NULL;
#endif
    // 初始化头节点的其他元素
    list->count = 0;
    list->cur_level = 1;
    list->max_level = max_level;
    return 0;
}

// 销毁跳表：
// 返回值：0成功，-1失败
int skiplist_desy(skiplist_t *list){
    if(list == NULL) return -1;
    // 删除所有数据节点
    skiplist_node_t* cur_node = list->header->next[0];
    skiplist_node_t* nxt_node = cur_node->next[0];
    while(cur_node != NULL){
        // 调整信息
        for(int i=0; i<list->max_level; i++){
            list->header->next[i] = cur_node->next[i];
        }
        list->count--;
        // 开始删除
        nxt_node = cur_node->next[0];
        if(skiplist_node_desy(cur_node) != 0){
            return -1;
        }
        cur_node = nxt_node;
    }
    // 删除头节点
    if(list->header){
        free(list->header->next);
        list->header->next = NULL;
        free(list->header);
        list->header = NULL;
    }
    list->count = 0;
    list->cur_level = 0;
    return 0;
}

// 插入元素：有冲突则是头插法
// 返回值：0成功，-1失败，-2已经有相应的key
int skiplist_node_insert(skiplist_t* list, Z_KEY_TYPE key, Z_VALUE_TYPE value){
    // 寻找新节点应该插入的位置
    skiplist_node_t* update[list->max_level];  // 查找的路径
    skiplist_node_t* p = list->header;
    for(int i=list->cur_level-1; i>=0; i--){
    #if KV_ZTYPE_INT_INT
        while(p->next[i]!=NULL && p->next[i]->key<key)
    #elif KV_ZTYPE_CHAR_CHAR
        while(p->next[i]!=NULL && strcmp(p->next[i]->key, key)<0)
    #endif
        {
            p = p->next[i];
        }
        update[i] = p;
    }
    // 将节点插入
#if KV_ZTYPE_INT_INT
    if(p->next[0]!=NULL && p->next[0]->key==key)
#elif KV_ZTYPE_CHAR_CHAR
    if(p->next[0]!=NULL && strcmp(p->next[0]->key, key)==0)
#endif
    {
        return -2;  // already have same key
    }else{
        // 新节点的层数--概率0.5
        int newlevel = 1;
        while((rand()%2) && newlevel<list->max_level){
            ++newlevel;
        }
        // 创建新节点
        skiplist_node_t* new_node = skiplist_node_create(key, value, newlevel);
        if(new_node == NULL) return -1;
        // 完善当前层级之上的查找路径（也就是头节点）
        if(newlevel > list->cur_level){
            for(int i=list->cur_level; i<newlevel; i++){
                update[i] = list->header;
            }
            list->cur_level = newlevel;
        }
        // 更新新节点的前后指向
        for(int i=0; i<newlevel; i++){
            new_node->next[i] = update[i]->next[i];
            update[i]->next[i] = new_node;
        }
        list->count++;
        return 0;
    }
}

// 查找元素
// 返回值：非空成功返回节点，NULL没找到
skiplist_node_t* skiplist_node_search(skiplist_t* list, Z_KEY_TYPE key){
    // 头节点开始找
    skiplist_node_t* p = list->header;
    for(int i=list->cur_level-1; i>=0; i--) {
    #if KV_ZTYPE_INT_INT
        while(p->next[i]!=NULL && p->next[i]->key<key)
    #elif KV_ZTYPE_CHAR_CHAR
        while(p->next[i]!=NULL && strcmp(p->next[i]->key, key)<0)
    #endif
        {
            p = p->next[i];
        }
    }
    // 判断下一个元素是否是当前key
#if KV_ZTYPE_INT_INT
    if(p->next[0]!=NULL && p->next[0]->key==key)
#elif KV_ZTYPE_CHAR_CHAR
    if(p->next[0]!=NULL && strcmp(p->next[0]->key, key)==0)
#endif
    {
        return p->next[0];
    }else{
        return NULL;
    }
}

// 删除元素
// 返回值：0成功，-1失败，-2没有
int skiplist_node_delete(skiplist_t* list, Z_KEY_TYPE key){
    // 查找节点
    skiplist_node_t* update[list->max_level];
    skiplist_node_t* p = list->header;
    for(int i=list->cur_level-1; i>=0; i--){
    #if KV_ZTYPE_INT_INT
        while(p->next[i]!=NULL && p->next[i]->key<key)
    #elif KV_ZTYPE_CHAR_CHAR
        while(p->next[i]!=NULL && strcmp(p->next[i]->key, key)<0)
    #endif
        {
            p = p->next[i];
        }
        update[i] = p;
    }
    // 删除节点并更新指向信息
#if KV_ZTYPE_INT_INT
    if(p->next[0]!=NULL && p->next[0]->key==key)
#elif KV_ZTYPE_CHAR_CHAR
    if(p->next[0]!=NULL && strcmp(p->next[0]->key, key)==0)
#endif
    {
        skiplist_node_t* node_d = p->next[0];  // 待删除元素
        for(int i=0; i<list->cur_level; i++){
            if(update[i]->next[i] == node_d){
                update[i]->next[i] = node_d->next[i];
            }
        }
        int ret = skiplist_node_desy(node_d);
        if(ret == 0){
            list->count--;
            for(int i=0; i<list->max_level; i++){
                if(list->header->next[i] == NULL){
                    list->cur_level = i;
                    break;
                }
            }
            
        }
        return ret;
    }else{
        return -2;  // no such key
    }
}

// 打印整个跳表
// 返回值：0成功，-1失败
int skiplist_print(skiplist_t* list){
    if(list==NULL) return -1;
    skiplist_node_t* p;
    for (int i=list->cur_level-1; i>=0; i--){
        printf("Level %d:", i);
        p = list->header->next[i];
        while (p != NULL) {
    #if KV_ZTYPE_INT_INT
        printf(" (%d,%d)", p->key, p->value);
        printf(" %d", p->key);
    #elif KV_ZTYPE_CHAR_CHAR
        // printf(" (%s,%s)", p->key, p->value);
        printf(" %s", p->key);
    #endif
            p = p->next[i];
        }
        printf("\n");
    }
    printf("\n");
    return 0;
}
/*------------------------------------------------------------------*/


/*----------------------------kv存储协议-----------------------------*/
// 初始化
// 参数：kv_z要传地址
// 返回值：0成功，-1失败
int kv_skiplist_init(kv_skiplist_t* kv_z, int m){
    if(kv_z==NULL || m<=0) return -1;
    return skiplist_init(kv_z, m);;
}
// 销毁
// 参数：kv_z要传地址
// 返回值：0成功，-1失败
int kv_skiplist_desy(kv_skiplist_t* kv_z){
    if(kv_z==NULL) return -1;
    return skiplist_desy(kv_z);
}
// 插入指令：有就报错，没有就创建
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_skiplist_set(kv_skiplist_t* kv_z, char** tokens){
    if(kv_z==NULL || tokens==NULL || tokens[1]==NULL || tokens[2]==NULL) return -1;
    return skiplist_node_insert(kv_z, tokens[1], tokens[2]);
}
// 查找指令
// 返回值：正常返回value，NULL表示没有
char* kv_skiplist_get(kv_skiplist_t* kv_z, char** tokens){
    if(kv_z==NULL || tokens==NULL || tokens[1]==NULL) return NULL;
    skiplist_node_t* node = skiplist_node_search(kv_z, tokens[1]);
    if(node != NULL){
        return node->value;
    }else{
        return NULL;
    }
}
// 删除指令
// 返回值：0成功，-1失败，-2没有
int kv_skiplist_delete(kv_skiplist_t* kv_z, char** tokens){
    if(kv_z==NULL || tokens==NULL || tokens[1]==NULL) return -1;
    return skiplist_node_delete(kv_z, tokens[1]);
}
// 计数指令
int kv_skiplist_count(kv_skiplist_t* kv_z){
    return kv_z->count;
}
// 存在指令
// 返回值：1存在，0没有
int kv_skiplist_exist(kv_skiplist_t* kv_z, char** tokens){
    return (skiplist_node_search(kv_z, tokens[1]) != NULL);
}
/*------------------------------------------------------------------*/


/*-----------------------------测试代码------------------------------*/
#if ENABLE_SKIPLIST_DEBUG
#if KV_ZTYPE_INT_INT
int main(){
    return 0;
}
#elif KV_ZTYPE_CHAR_CHAR
int main(){
    return 0;
}
#endif
#endif
/*------------------------------------------------------------------*/
