/*
 * 本代码实现kv存储中的array数组存储结构
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"array.h"

/*-----------------------------函数声明------------------------------*/
// array遍历查找
// 参数说明：
//  cur_blk：传一个没有定义的块！用于返回当前kv存储对所在的块
// 返回值：正常返回键值对，NULL表示没有
kv_array_item_t* kv_array_search(kv_array_t* kv_a, const char* key, kv_array_block_t* cur_blk);// 在最后创建一个块
// 返回值：正常返回最后一个块，NULL表示创建失败
kv_array_block_t* kv_array_create_block(kv_array_t* kv_a);
// 释放给定的块
// 返回值：0成功，-1失败
int kv_array_free_block(kv_array_t* kv_a, kv_array_block_t* blk);
// 找到第一个空节点，若全满了就创建一个新的块
// 参数说明：
//  kv_a：array数据类型的头
//  cur_blk：传一个没有定义的块！用于返回当前kv存储对所在的块
kv_array_item_t* kv_array_find_null(kv_array_t* kv_a, kv_array_block_t* cur_blk);

// 初始化
// 参数：kv_rb要传地址
// 返回值：0成功，-1失败
int kv_array_init(kv_array_t* kv_a);
// 销毁
// 参数：T要传地址
// 返回值：0成功，-1失败
int kv_array_desy(kv_array_t* kv_a);
// 插入指令
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_array_set(kv_array_t* kv_a, char** tokens);
// 查找指令
char* kv_array_get(kv_array_t* kv_a, char** tokens);
// 删除指令，另外若当前块为空就释放当前块
// 返回值：0成功，-1失败，-2没有
int kv_array_delete(kv_array_t* kv_a, char** tokens);
// 计数指令
int kv_array_count(kv_array_t* kv_a);
// 存在指令
// 返回值：1存在，0没有
int kv_array_exist(kv_array_t* kv_a, char** tokens);
/*------------------------------------------------------------------*/


/*-----------------------------函数定义------------------------------*/
// array遍历查找
// 参数说明：
//  cur_blk：传一个没有定义的块！用于返回当前kv存储对所在的块
// 返回值：正常返回键值对，NULL表示没有
kv_array_item_t* kv_array_search(kv_array_t* kv_a, const char* key, kv_array_block_t* cur_blk){
    if(kv_a==NULL || key==NULL) return NULL;
    
    cur_blk = kv_a->head;
    while (cur_blk != NULL){
        for(int idx = 0; idx<kv_array_block_size; idx++){
            if(cur_blk->items[idx].key != NULL && 0 == strcmp(cur_blk->items[idx].key, key)){
                return &(cur_blk->items[idx]);
            }
        }
        cur_blk = cur_blk->next;
    }
    return NULL;
}

// 在最后创建一个块
// 返回值：正常返回最后一个块，NULL表示创建失败
kv_array_block_t* kv_array_create_block(kv_array_t* kv_a){
    // 定位到最后一个块
    kv_array_block_t* cur_blk = kv_a->head;
    while(cur_blk != NULL && cur_blk->next != NULL){
        cur_blk = cur_blk->next;
    }
    // 创建块
    kv_array_block_t* end_blk = (kv_array_block_t*)calloc(1, sizeof(kv_array_block_t));
    if(end_blk == NULL){
        return NULL;
    }
    end_blk->items = (kv_array_item_t*)calloc(kv_array_block_size, sizeof(kv_array_item_t));
    if(end_blk->items == NULL){
        free(end_blk);
        end_blk = NULL;
        return NULL;
    }
    end_blk->next = NULL;
    end_blk->count = 0;
    // 将这个块添加进来并返回
    if(kv_a->head != NULL){
        cur_blk->next = end_blk;
    }else{
        kv_a->head = end_blk;
    }
    return end_blk;
}

// 释放给定的块
// 返回值：0成功，-1失败
int kv_array_free_block(kv_array_t* kv_a, kv_array_block_t* blk){
    // 当前块还有kv时不能删除
    if(blk->count != 0){
        return -1;
    }
    // 当前块是第一个块不删除
    if(kv_a->head == blk){
        return -1;
    }
    // 找到当前块的前一个块
    kv_array_block_t* last_blk = kv_a->head;
    while(last_blk->next != blk && last_blk->next != NULL){
        last_blk = last_blk->next;
    }
    if(last_blk->next != NULL){
        return -1;  // 没有这个块
    }
    last_blk->next = blk->next;
    // 释放当前块
    if(blk->items){
        free(blk->items);
        blk->items = NULL;
    }
    blk->next = NULL;
    blk->count = 0;
    if(blk){
        free(blk);
        blk = NULL;
    }
    return 0;
}

// 找到第一个空节点，若全满了就创建一个新的块
// 参数说明：
//  kv_a：array数据类型的头
//  cur_blk：传一个没有定义的块！用于返回当前kv存储对所在的块
kv_array_item_t* kv_array_find_null(kv_array_t* kv_a, kv_array_block_t* cur_blk){
    cur_blk = kv_a->head;
    // 如果一个都没有，就创建第一个块
    if(cur_blk == NULL){
        cur_blk = kv_array_create_block(kv_a);
        if(cur_blk == NULL) return NULL;
        return &(cur_blk->items[0]);
    }
    // 从第一个块依次开始找
    // kv_array_block_t* cur_blk = kv_a->head;
    kv_array_block_t* last_blk = kv_a->head;
    // 寻找所有非空块的空闲位置
    while (cur_blk != NULL){
        for(int idx=0; idx<kv_array_block_size; idx++){
            if(cur_blk->items[idx].key == NULL && cur_blk->items[idx].value == NULL){
                return &(cur_blk->items[idx]);
            }
        }
        last_blk = cur_blk;
        cur_blk = cur_blk->next;
    }
    // 都没有就创建一个新的块
    cur_blk = kv_array_create_block(kv_a);
    if(cur_blk == NULL) return NULL;
    last_blk->next = cur_blk;
    return &(cur_blk->items[0]);
}

// 初始化
// 参数：kv_rb要传地址
// 返回值：0成功，-1失败
int kv_array_init(kv_array_t* kv_a){
    kv_array_block_t* cur_blk = kv_array_create_block(kv_a);
    if(cur_blk == NULL) return -1;
    kv_a->head = cur_blk;
    kv_a->count = 0;
    return 0;
}
// 销毁
// 参数：T要传地址
// 返回值：0成功，-1失败
int kv_array_desy(kv_array_t* kv_a){
    kv_array_block_t* cur_blk = kv_a->head;
    kv_array_block_t* next_blk = cur_blk->next;
    while(cur_blk != NULL){
        next_blk = cur_blk->next;
        if(cur_blk->items){
            free(cur_blk->items);
            cur_blk->items = NULL;
        }
        if(cur_blk){
            free(cur_blk);
            cur_blk = NULL;
        }
        cur_blk = next_blk;
    }
    return 0;
}

// 插入指令
// 返回值：0表示成功、-1表示失败、-2表示已经有key
int kv_array_set(kv_array_t* kv_a, char** tokens){
    if(kv_a==NULL || tokens[1]==NULL || tokens[2]==NULL) return -1;
    // 若当前数组存在直接返回
    if(kv_array_search(kv_a, tokens[1], NULL) != NULL){
        return -2;
    }
    // 复制key
    char* kcopy = (char*)malloc(strlen(tokens[1])+1);
    if(kcopy == NULL) return -1;
    strncpy(kcopy, tokens[1], strlen(tokens[1])+1);
    // 复制value
    char* vcopy = (char*)malloc(strlen(tokens[2])+1);
    if(vcopy == NULL){
        free(kcopy);
        return -1;
    }
    strncpy(vcopy, tokens[2], strlen(tokens[2])+1);
    // 找到数组中的第一个空索引，并存储kv
    kv_array_block_t blk;
    kv_array_item_t* item = kv_array_find_null(kv_a, &blk);
    if(item == NULL) return -1;
    item->key = kcopy;
    item->value = vcopy;
    kv_a->count++;
    blk.count++;
    return 0;
}

// 查找指令
char* kv_array_get(kv_array_t* kv_a, char** tokens){
    kv_array_item_t* item = kv_array_search(kv_a, tokens[1], NULL);
    if(item != NULL){
        return item->value;
    }else{
        return NULL;
    }
}

// 删除指令，另外若当前块为空就释放当前块
// 返回值：0成功，-1失败，-2没有
int kv_array_delete(kv_array_t* kv_a, char** tokens){
    kv_array_block_t blk;
    kv_array_item_t* item = kv_array_search(kv_a, tokens[1], &blk);
    if(item == NULL){
        return -2;
    }else{
        if(item->value){
            free(item->value);
            item->value = NULL;
        }
        if(item->key){
            free(item->key);
            item->key = NULL;
        }
        kv_a->count--;
        blk.count--;
        if(blk.count==0 && kv_a->head!=&blk){
            kv_array_free_block(kv_a, &blk);
        }
        return 0;
    }
}

// 计数指令
int kv_array_count(kv_array_t* kv_a){
    return kv_a->count;
}

// 存在指令
// 返回值：1存在，0没有
int kv_array_exist(kv_array_t* kv_a, char** tokens){
    return (kv_array_search(kv_a, tokens[1], NULL) != NULL);
}
/*------------------------------------------------------------------*/


/*-----------------------------测试代码------------------------------*/
/*------------------------------------------------------------------*/
