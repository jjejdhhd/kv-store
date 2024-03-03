#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

// 编译指令：gcc -o main 1-1btree.c
// 本代码实现M阶B树，存储int型key，未定义value。

#define BTREE_DEBUG 1  // 是否运行测试代码

typedef struct _btree_node{
    int *keys;
    void *values;
    struct _btree_node **children;
    int num;  // 当前节点的实际元素数量
    int leaf; // 当前节点是否为叶子节点
}btree_node;

typedef struct _btree{
    int m;  // m阶B树
    struct _btree_node *root_node;
}btree;


/*
下面是所有的函数声明，排列顺序与源代码调用相同，最外层的函数放在最下面。
*/
/*----初始化分配内存----*/
// 创建单个节点，leaf表示是否为叶子节点
btree_node *btree_node_create(btree *T, int leaf);
// 初始化m阶B树：分配内存，最后记得销毁B树btree_destroy()
btree *btree_init(int m);

/*----释放内存----*/
// 删除单个节点
void btree_node_destroy(btree_node *cur);
// 递归删除给定节点作为根节点的子树
void btree_node_destroy_recurse(btree_node *cur);
// 删除所有节点，释放btree内存
btree *btree_destroy(btree *T);

/*----插入操作----*/
// 根节点分裂
btree_node* btree_root_split(btree *T);
// 索引为idx的孩子节点分裂
btree_node* btree_child_split(btree *T, btree_node* cur, int idx);
// btree插入元素：先分裂，再插入，必然在叶子节点插入
void btree_insert_key(btree *T, int key);

/*----删除操作----*/
// 借位：将cur节点的idx_key元素下放到idx_dest孩子
btree_node *btree_borrow(btree_node *cur, int idx_key, int idx_dest);
// 合并：将cur节点的idx元素向下合并
btree_node *btree_merge(btree *T, btree_node *cur, int idx);
// 找出当前节点索引为idx_key的元素的前驱节点
btree_node* btree_precursor_node(btree *T, btree_node *cur, int idx_key);
// 找出当前节点索引为idx_key的元素的后继节点
btree_node* btree_successor_node(btree *T, btree_node *cur, int idx_key);
// btree删除元素：先合并/借位，再删除，必然在叶子节点删除
void btree_delete_key(btree *T, int key);

/*----查找操作----*/
// 查找key
btree_node* btree_search_key(btree *T, int key);

/*----打印信息----*/
// 打印当前节点信息
void btree_node_print(btree_node *cur);
// 先序遍历给定节点为根节点的子树(递归)
void btree_traversal_node(btree *T, btree_node *cur);
// btree遍历
void btree_traversal(btree *T);

/*----检查有效性----*/
// 获取B树的高度
int btree_depth(btree *T);
// 检查给定节点的有效性
// 键值：根节点至少有一个key，其余节点至少有ceil(m/2)-1个key
// 分支：所有节点数目子树为当前节点元素数量+1
bool btree_node_check_effective(btree *T, btree_node *cur);
// 遍历所有路径检查m阶B树的有效性
// 平衡性：所有叶节点都在同一层（所有路径高度相等）
// 有序性：所有元素升序排序
// 键值：根节点至少有一个key，其余节点至少有ceil(m/2)-1个key
// 分支：所有节点数目子树为当前节点元素数量+1
bool btree_check_effective(btree *T);

/*-----------------------------下面为函数定义-------------------------------*/
// 创建单个节点，leaf表示是否为叶子节点
btree_node *btree_node_create(btree *T, int leaf){
    btree_node *new = (btree_node*)malloc(sizeof(btree_node));
    if(new == NULL){
        printf("btree node malloc failed!\n");
        return NULL;
    }
    new->keys = (int*)calloc(T->m-1, sizeof(int));
    new->values = (void*)calloc(T->m-1, sizeof(void));
    new->children = (btree_node **)calloc(T->m, sizeof(btree_node*));
    new->num = 0;
    new->leaf = leaf;
    return new;
}

// 删除单个节点
void btree_node_destroy(btree_node *cur){
    free(cur->keys);
    free(cur->values);
    free(cur->children);
    free(cur);
}

// 初始化m阶B树：分配内存，最后记得销毁B树btree_destroy()
btree *btree_init(int m){
    btree *T = (btree*)malloc(sizeof(btree));
    if(T == NULL){
        // 只有内存不够时才会分配失败
        printf("rbtree malloc failed!\n");
        return NULL;
    }
    T->m = m;
    T->root_node = NULL;
}

// 递归删除给定节点作为根节点的子树
void btree_node_destroy_recurse(btree_node *cur){
    int i = 0;
    if(cur->leaf == 1){
        btree_node_destroy(cur);
    }else{
        for(i=0; i<cur->num+1; i++){
            btree_node_destroy_recurse(cur->children[i]);
        }
    }
}

// 释放btree内存
btree *btree_destroy(btree *T){
    // 删除所有节点
    if(T->root_node != NULL){
        btree_node_destroy_recurse(T->root_node);
    }
    // 删除btree
    free(T);
}


// 根节点分裂
btree_node* btree_root_split(btree *T){
    // 创建兄弟节点
    btree_node *brother = btree_node_create(T, T->root_node->leaf);
    int i = 0;
    for(i=0; i<((T->m-1)>>1); i++){
        brother->keys[i] = T->root_node->keys[i+(T->m>>1)];
        T->root_node->keys[i+(T->m>>1)] = 0;
        brother->children[i] = T->root_node->children[i+(T->m>>1)];
        T->root_node->children[i+(T->m>>1)] = NULL;
        brother->num++;
        T->root_node->num--;
    }
    // 还需要复制最后一个指针
    brother->children[brother->num] = T->root_node->children[T->m-1];
    T->root_node->children[T->m-1] = NULL;
    
    // 创建新的根节点
    btree_node *new_root = btree_node_create(T, 0);
    new_root->keys[0] = T->root_node->keys[T->root_node->num-1];
    T->root_node->keys[T->root_node->num-1] = 0;
    T->root_node->num--;
    new_root->num = 1;
    new_root->children[0] = T->root_node;
    new_root->children[1] = brother;
    T->root_node = new_root;

    return T->root_node;
}

// 索引为idx的孩子节点分裂
btree_node* btree_child_split(btree *T, btree_node* cur, int idx){
    // 创建孩子的兄弟节点
    btree_node *full_child = cur->children[idx];
    btree_node *new_child = btree_node_create(T, cur->children[idx]->leaf);
    int i = 0;
    for(i=0; i<((T->m-1)>>1); i++){
        new_child->keys[i] = full_child->keys[i+(T->m>>1)];
        full_child->keys[i+(T->m>>1)] = 0;
        new_child->children[i] = full_child->children[i+(T->m>>1)];
        full_child->children[i+(T->m>>1)] = NULL;
        new_child->num++;
        full_child->num--;
    }
    new_child->children[new_child->num] = full_child->children[T->m-1];
    full_child->children[T->m-1] = NULL;

    // 把孩子的元素拿上来
    // 调整自己的key和children
    for(i=cur->num; i>idx; i--){
        cur->keys[i] = cur->keys[i-1];
        cur->children[i+1] = cur->children[i];
    }
    cur->children[idx+1] = new_child;
    cur->keys[idx] = full_child->keys[full_child->num-1];
    full_child->keys[full_child->num-1] = 0;
    cur->num++;
    full_child->num--;

    return cur;
}


// btree插入元素：先分裂，再插入，必然在叶子节点插入
void btree_insert_key(btree *T, int key){
    btree_node *cur = T->root_node;
    if(key <= 0){
        // printf("illegal insert: key=%d!\n", key);
    }else if(cur == NULL){
        btree_node *new = btree_node_create(T, 1);
        new->keys[0] = key;
        new->num = 1;
        T->root_node = new;
    }else{
    // 函数整体逻辑：从根节点逐步找到元素要插入的叶子节点，先分裂、再添加
        // 先查看根节点是否需要分裂
        if(cur->num == T->m-1){
            cur = btree_root_split(T);
        }

        // 从根节点开始寻找要插入的叶子节点
        while(cur->leaf == 0){
            // 找到下一个要比较的孩子节点
            int next_idx = 0;  // 要进入的孩子节点的索引
            int i = 0;
            for(i=0; i<cur->num; i++){
                if(key == cur->keys[i]){
                    // printf("insert failed! already has key=%d!\n", key);
                    return;
                }else if(key < cur->keys[i]){
                    next_idx = i;
                    break;
                }else if(i == cur->num-1){
                    next_idx = cur->num;
                }
            }
            // 查看孩子是否需要分裂，不需要就进入
            if(cur->children[next_idx]->num == T->m-1){
                cur = btree_child_split(T, cur, next_idx);
            }else{
                cur = cur->children[next_idx];
            }
        }

        // 将新元素插入到叶子节点中
        int i = 0;
        int pos = 0;  // 要插入的位置
        for(i=0; i<cur->num; i++){
            if(key == cur->keys[i]){
                // printf("insert failed! already has key=%d!\n", key);
                return;
            }else if(key < cur->keys[i]){
                pos = i;
                break;
            }else if(i == cur->num-1){
                pos = cur->num;
            }
        }
        // 插入元素
        if(pos == cur->num){
            cur->keys[cur->num] = key;
        }else{
            for(i=cur->num; i>pos; i--){
                cur->keys[i] = cur->keys[i-1];
            }
            cur->keys[pos] = key;
        }
        cur->num++;
    }
}

// 借位：将cur节点的idx_key元素下放到idx_dest孩子
btree_node *btree_borrow(btree_node *cur, int idx_key, int idx_dest){
    int idx_sour = (idx_key == idx_dest) ? idx_dest+1 : idx_key;
    btree_node *node_dest = cur->children[idx_dest];  // 目的节点
    btree_node *node_sour = cur->children[idx_sour];  // 源节点
    if(idx_key == idx_dest){
        // 自己下去作为目的节点的最后一个元素
        node_dest->keys[node_dest->num] = cur->keys[idx_key];
        node_dest->children[node_dest->num+1] = node_sour->children[0];
        node_dest->num++;
        // 把源节点的第一个元素请上来
        cur->keys[idx_key] = node_sour->keys[0];
        for(int i=0; i<node_sour->num-1; i++){
            node_sour->keys[i] = node_sour->keys[i+1];
            node_sour->children[i] = node_sour->children[i+1];
        }
        node_sour->children[node_sour->num-1] = node_sour->children[node_sour->num];
        node_sour->children[node_sour->num] = NULL;
        node_sour->keys[node_sour->num-1] = 0;
        node_sour->num--;
    }else{
        // 自己下去作为目的节点的第一个元素
        node_dest->children[node_dest->num+1] = node_dest->children[node_dest->num];
        for(int i=node_dest->num; i>0; i--){
            node_dest->keys[i] = node_dest->keys[i-1];
            node_dest->children[i] = node_dest->children[i-1];
        }
        node_dest->keys[0] = cur->keys[idx_key];
        node_dest->children[0] = node_sour->children[node_sour->num];
        node_dest->num++;
        // 把源节点的最后一个元素请上来
        cur->keys[idx_key] = node_sour->keys[node_sour->num-1];
        node_sour->keys[node_sour->num-1] = 0;
        node_sour->children[node_sour->num] = NULL;
        node_sour->num--;
    }
    return node_dest;
}

// 合并：将cur节点的idx元素向下合并
btree_node *btree_merge(btree *T, btree_node *cur, int idx){
    btree_node *left = cur->children[idx];
    btree_node *right = cur->children[idx+1];
    // 自己下去左孩子，调整当前节点
    left->keys[left->num] = cur->keys[idx];
    left->num++;
    for(int i=idx; i<cur->num-1; i++){
        cur->keys[i] = cur->keys[i+1];
        cur->children[i+1] = cur->children[i+2];
    }
    cur->keys[cur->num-1] = 0;
    cur->children[cur->num] = NULL;
    cur->num--;
    // 右孩子复制到左孩子
    for(int i=0; i<right->num; i++){
        left->keys[left->num] = right->keys[i];
        left->children[left->num] = right->children[i];
        left->num++;
    }
    left->children[left->num] = right->children[right->num];
    // 删除右孩子
    btree_node_destroy(right);
    // 更新根节点
    if(T->root_node == cur){
        btree_node_destroy(cur);
        T->root_node = left;
    }
    return left;
}

// 找出当前节点索引为idx_key的元素的前驱节点
btree_node* btree_precursor_node(btree *T, btree_node *cur, int idx_key){
    if(cur->leaf == 0){
        cur = cur->children[idx_key];
        while(cur->leaf == 0){
            cur = cur->children[cur->num];
        }
    }
    return cur;
}

// 找出当前节点索引为idx_key的元素的后继节点
btree_node* btree_successor_node(btree *T, btree_node *cur, int idx_key){
    if(cur->leaf == 0){
        cur = cur->children[idx_key+1];
        while(cur->leaf == 0){
            cur = cur->children[0];
        }
    }
    return cur;
}


// btree删除元素：先合并/借位，再删除，必然在叶子节点删除
void btree_delete_key(btree *T, int key){
    if(T->root_node!=NULL && key>0){
        btree_node *cur = T->root_node;
        // 在去往叶子节点的过程中不断调整(合并/借位)
        while(cur->leaf == 0){
            // 看看要去哪个孩子
            int idx_next = 0; //下一个要去的孩子节点索引
            int idx_bro = 0;
            int idx_key = 0;
            if(key < cur->keys[0]){
                idx_next = 0;
                idx_bro = 1;
            }else if(key > cur->keys[cur->num-1]){
                idx_next = cur->num;
                idx_bro = cur->num-1;
            }else{
                for(int i=0; i<cur->num; i++){
                    if(key == cur->keys[i]){
                        // 哪边少去哪边
                        if(cur->children[i]->num <= cur->children[i+1]->num){
                            idx_next = i;
                            idx_bro = i+1;
                        }else{
                            idx_next = i+1;
                            idx_bro = i;
                        }
                        break;
                    }else if((i<cur->num-1) && (key > cur->keys[i]) && (key < cur->keys[i+1])){
                        idx_next = i + 1;
                        // 谁多谁是兄弟
                        if(cur->children[i]->num > cur->children[i+2]->num){
                            idx_bro = i;
                        }else{
                            idx_bro = i+2;
                        }
                        break;
                    }
                }
            }
            idx_key = (idx_next < idx_bro) ? idx_next : idx_bro;
            // 依据孩子节点的元素数量进行调整
            if(cur->children[idx_next]->num <= ((T->m>>1)-1)){
                // 借位：下一孩子的元素少，下一孩子的兄弟节点的元素多
                if(cur->children[idx_bro]->num >= (T->m>>1)){
                    cur = btree_borrow(cur, idx_key, idx_next);
                }
                // 合并：两个孩子都不多
                else{
                    cur = btree_merge(T, cur, idx_key);
                }
            }else if(cur->keys[idx_key] == key){
                // 若当前元素就是要删除的节点，那一定要送下去
                // 但是不能借位,而是将前驱元素搬上来
                btree_node* pre;
                int tmp;
                if(idx_key == idx_next){
                    // 找到前驱节点
                    pre = btree_precursor_node(T, cur, idx_key);
                    // 交换 当前元素 和 前驱节点的最后一个元素
                    tmp = pre->keys[pre->num-1];
                    pre->keys[pre->num-1] = cur->keys[idx_key];
                    cur->keys[idx_key] = tmp;
                }else{
                    // 找到后继节点
                    pre = btree_successor_node(T, cur, idx_key);
                    // 交换 当前元素 和 后继节点的第一个元素
                    tmp = pre->keys[0];
                    pre->keys[0] = cur->keys[idx_key];
                    cur->keys[idx_key] = tmp;
                }
                cur = cur->children[idx_next];
                // cur = btree_borrow(cur, idx_key, idx_next);
            }else{
                cur = cur->children[idx_next];
            }
        }
        // 叶子节点删除元素
        for(int i=0; i<cur->num; i++){
            if(cur->keys[i] == key){
                if(cur->num == 1){
                    // 若B树只剩最后一个元素
                    btree_node_destroy(cur);
                    T->root_node = NULL;
                }else{
                    if(i != cur->num-1){
                        for(int j=i; j<(cur->num-1); j++){
                            cur->keys[j] = cur->keys[j+1];
                        }
                    }
                    cur->keys[cur->num-1] = 0;
                    cur->num--;
                }
            }
            // else if(i == cur->num-1){
            //     printf("there is no key=%d\n", key);
            // }
        }
    }
}

// 打印当前节点信息
void btree_node_print(btree_node *cur){
    if(cur == NULL){
        printf("NULL\n");
    }else{
        printf("leaf:%d, num:%d, key:|", cur->leaf, cur->num);
        for(int i=0; i<cur->num; i++){
            printf("%d|", cur->keys[i]);
        }
        printf("\n");
    }
}

// 先序遍历给定节点为根节点的子树(递归)
void btree_traversal_node(btree *T, btree_node *cur){
    // 打印当前节点信息
    btree_node_print(cur);

    // 依次打印所有子节点信息
    if(cur->leaf == 0){
        int i = 0;
        for(i=0; i<cur->num+1; i++){
            btree_traversal_node(T, cur->children[i]);
        }
    }
}

// btree遍历
void btree_traversal(btree *T){
    if(T->root_node != NULL){
        btree_traversal_node(T, T->root_node);
    }else{
        // printf("btree_traversal(): There is no key in B-tree!\n");
    }
}

// 查找key
btree_node* btree_search_key(btree *T, int key){
    if(key > 0){
        btree_node *cur = T->root_node;
        // 先寻找是否为非叶子节点
        while(cur->leaf == 0){
            if(key < cur->keys[0]){
                cur = cur->children[0];
            }else if(key > cur->keys[cur->num-1]){
                cur = cur->children[cur->num];
            }else{
                for(int i=0; i<cur->num; i++){
                    if(cur->keys[i] == key){
                        return cur;
                    }else if((i<cur->num-1) && (key > cur->keys[i]) && (key < cur->keys[i+1])){
                        cur = cur->children[i+1];
                    }
                }
            }
        }
        // 在寻找是否为叶子节点
        if(cur->leaf == 1){
            for(int i=0; i<cur->num; i++){
                if(cur->keys[i] == key){
                    return cur;
                }
            }
        }
    }
    // 都没找到返回NULL
    return NULL;
}

// 获取B树的高度
int btree_depth(btree *T){
    int depth = 0;
    btree_node *cur = T->root_node;
    while(cur != NULL){
        depth++;
        cur = cur->children[0];
    }
    return depth;
}

// 检查给定节点的有效性
// 键值：根节点至少有一个key，其余节点至少有ceil(m/2)-1个key
// 分支：所有节点数目子树为当前节点元素数量+1
bool btree_node_check_effective(btree *T, btree_node *cur){
    bool eff_flag = true;
    // 统计键值和子节点数量
    int num_kvs = 0, num_child = 0;
    int i = 0;
    while(cur->keys[i] != 0){
        // 判断元素是否递增
        if(i>=1 && (cur->keys[i] <= cur->keys[i-1])){
            printf("ERROR! the following node DOT sorted!\n");
            btree_node_print(cur);
            eff_flag = false;
            break;
        }
        // 统计数量
        num_kvs++;
        i++;
    }
    i = 0;
    while(cur->children[i] != NULL){
        // 子节点和当前节点的有序性
        if(i<num_kvs){
            if(cur->keys[i] <= cur->children[i]->keys[cur->children[i]->num]){
                printf("ERROR! the follwing node's child[%d] has bigger key=%d than %d\n", i, cur->children[i]->keys[cur->children[i]->num], cur->keys[i]);
                printf("follwing node--");
                btree_node_print(cur);
                printf("  error child--");
                btree_node_print(cur->children[i]);
                eff_flag = false;
            }else if(cur->keys[i] >= cur->children[i+1]->keys[0]){
                printf("ERROR! the follwing node's child[%d] has smaller key=%d than %d\n", i+1, cur->children[i+1]->keys[0], cur->keys[i]);
                printf("follwing node--");
                btree_node_print(cur);
                printf("  error child--");
                btree_node_print(cur->children[i+1]);
                eff_flag = false;
            }
        }
        // 统计数量
        num_child++;
        i++;
    }
    // 判断元素数量是否合理
    if(cur->num >= T->m){
        printf("ERROR! the follwing node has too much keys:%d(at most %d)\n", cur->num, T->m-1);
        btree_node_print(cur);
        eff_flag = false;
    }
    if((cur != T->root_node) && (num_kvs<((T->m>>1)-1))){
        printf("ERROR! the follwing node has too few keys:%d(at least %d)\n", num_kvs, (T->m>>1)-1);
        btree_node_print(cur);
        eff_flag = false;
    }
    if(num_kvs != cur->num){
        printf("ERROR! the follwing node has %d keys but num=%d\n", num_kvs, cur->num);
        btree_node_print(cur);
        eff_flag = false;
    }
    if((cur->leaf == 0) && (num_child != cur->num+1)){
        printf("ERROR! the follwing node has %d keys but %d child(except keys+1=child)\n", num_kvs, num_child);
        btree_node_print(cur);
        eff_flag = false;
    }
    return eff_flag;
}

// 遍历所有路径检查m阶B树的有效性
// 平衡性：所有叶节点都在同一层（所有路径高度相等）
// 有序性：所有元素升序排序
// 键值：根节点至少有一个key，其余节点至少有ceil(m/2)-1个key
// 分支：所有节点数目子树为当前节点元素数量+1
bool btree_check_effective(btree *T){
    bool effe_flag = true;
    int depth = btree_depth(T);
    if(depth == 0){
        // printf("btree_check_effective(): There is no key in B-tree!\n");
    }else if(depth == 1){
        // 只有一个根节点
        effe_flag = btree_node_check_effective(T, T->root_node);
    }else{
        // 最大的可能路径数量
        int max_path = 1;
        int depth_ = depth-1;
        while(depth_ != 0){
            max_path *= T->m;
            depth_--;
        }
        // 遍历所有路径(每个路径对应一个叶子节点)
        btree_node *cur = T->root_node;
        int i_path = 0;
        for(i_path=0; i_path<max_path; i_path++){
            int dir = i_path;  // 本次路径的方向控制
            int i_height = 0;  // 本次路径的高度
            int i_effe = 1; // 指示是否存在本路径
            cur = T->root_node;
            while(cur != NULL){
                // 当前节点的有效性
                effe_flag = btree_node_check_effective(T, cur);
                if(!effe_flag) break;
                // 更新高度
                i_height++;
                // 更新下一节点
                if(cur->children[dir%T->m]==NULL && !cur->leaf){
                    i_effe = 0;
                    break;
                }
                cur = cur->children[dir%T->m];
                dir /= T->m;
            }
            // if(btree_node_check_effective(T, cur))

            // 判断本路径节点数（高度）
            if(i_height != depth && i_effe){
                printf("ERROR! not all leaves in the same layer! the leftest path's height=%d, while the %dst path's height=%d.\n",
                       depth, i_path, i_height);
                effe_flag = false;
            }
            if(!effe_flag) break;
        }
        
    }
    return effe_flag;
}


/*-----------------------------下面为测试代码-------------------------------*/
#if BTREE_DEBUG
#include<time.h>  // 使用随机数
#include<sys/time.h>  // 计算qps中获取时间
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
#define ENABLE_QPS  1  // 是否开启qps性能测试
#define continue_test_len  10000000  // 连续测试的长度
// 冒泡排序
void bubble_sort(int arr[], int len) {
    int i, j, temp;
    for (i = 0; i < len - 1; i++)
        for (j = 0; j < len - 1 - i; j++)
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
}
// 打印当前数组
void print_int_array(int* KeyArray, int len_array){
    printf("测试数组为KeyArray[%d] = {", len_array);
    for(int i=0; i<len_array; i++){
        if(i == len_array-1){
            printf("%d", KeyArray[i]);
        }else{
            printf("%d, ", KeyArray[i]);
        }
    }
    printf("}\n");
}
int main(){
    // 定义
    /* --------------------定义数组-------------------- */
    // 预定义的数组
    // int KeyArray[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};  // 正着插
    // int KeyArray[20] = {20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};  // 倒着插
    // int KeyArray[20] = {1,2,3,4,5,10,9,8,7,6,11,12,13,14,15,20,19,18,17,16};  // 乱序插
    // int KeyArray[31] = {11,12,13,14,15,16,17,18,19,20,1,2,3,4,5,6,7,8,9,10,21,22,23,24,25,26,27,28,29,30,31};  // 乱序插
    // int KeyArray[18] = {18,8,13,9,13,0,7,13,14,7,1,7,19,7,9,18,17,18};  // 乱序插

    // // 顺序增长的数组
    // int len_array = 26;
    // int KeyArray[len_array];
    // int i_array = 0;
    // for(i_array=0; i_array<len_array; i_array++){
    //     KeyArray[i_array] = i_array + 1;
    // }

    // // 随机生成固定大小的随机数组
    // int len_array = 18;
    // int KeyArray[len_array];
    // srand(time(NULL));
    // int i_array = 0;
    // for(i_array=0; i_array<len_array; i_array++){
    //     // KeyArray[i_array] = rand() % 9999999999;
    //     KeyArray[i_array] = rand() % 20;
    // }
    // printf("RAND_MAX: %d\n", RAND_MAX);

    /* ------------------以下测试代码------------------ */
    // printf("-------------------B树插入测试------------------\n");
    // // 先给输入的数组排个序
    // int len_max = sizeof(KeyArray)/sizeof(int);
    // printf("测试数组长度: %d\n", len_max);
    // // 将原先的数组深拷贝并升序排序
    // int *KeyArray_sort = (int*)malloc(sizeof(KeyArray));
    // int i = 0;
    // for(i = 0; i < len_max; i++){
    //     KeyArray_sort[i] = KeyArray[i];
    // }
    // bubble_sort(KeyArray_sort, len_max);

    int max_test = 100;         // 测试的总次数
    int len_array = 1000;      // 单次测试的数组长度
    bool detail_flag = false;  // 是否打印详细信息
    bool pass_flag = true;
    printf("---------------------常规测试---------------------\n");
    for(int i_test=0; i_test<max_test; i_test++){
        // 随机生成固定大小的随机数组
        int KeyArray[len_array];
        srand(time(NULL));
        for(int i_array=0; i_array<len_array; i_array++){
            // KeyArray[i_array] = rand() % 9999999999;
            KeyArray[i_array] = rand() % len_array;
        }
        // int KeyArray[10000] = {};

        // 申请红黑树内存
        btree *T = btree_init(6);
        btree *T_old = btree_init(6);

        if(detail_flag){
            printf("--------------------开始测试--------------------\n");
            print_int_array(KeyArray, len_array);
        }
        
        if(detail_flag){
            printf("-------------------B树插入测试------------------\n");
        }
        /*-------------------B树插入测试------------------*/
        // 依次插入数据，并检查有效性
        for(int i=0; i<len_array; i++){
            if(i>0){
                btree_insert_key(T_old, KeyArray[i-1]);
            }
            btree_insert_key(T, KeyArray[i]);
            // 方便打印调试
            // if(i==65){
            //     printf("Before insert the %2d's key=%d:\n", i+1, KeyArray[i]);
            //     btree_traversal(T_old);
            //     printf("After insert the %2d's key=%d:\n", i+1, KeyArray[i]);
            //     btree_traversal(T);
            // }
            if(btree_check_effective(T)==false){
                printf("after insert KeyArray[%d]=%d error!\n", i, KeyArray[i]);
                pass_flag = false;
                break;
            }
        }
        if(pass_flag){
            if(detail_flag) printf("PASS---->插入测试%d/%d\n", i_test+1, max_test);
            btree_insert_key(T_old, KeyArray[len_array]);
        }else{
            if(detail_flag) printf("FAIL---->插入测试%d/%d\n", i_test+1, max_test);
            // printf("Before insert:\n");
            // btree_traversal(T_old);
            // printf("After insert:\n");
            // btree_traversal(T);

            btree_destroy(T_old);
            btree_destroy(T);
            break;
        }
        if(detail_flag){
            btree_traversal(T);
            printf("\n");
        }
        
        if(detail_flag){
            printf("-------------------B树查找测试------------------\n");
        }
        /*-------------------B树查找测试------------------*/
        btree_node* sear = NULL;
        for(int i=0; i<len_array; i++){
            if(KeyArray[i] > 0){
                sear = btree_search_key(T, KeyArray[i]);
                pass_flag = false;
                if(sear != NULL){
                    for(int j=0; j<sear->num; j++){
                        if(sear->keys[j] == KeyArray[i]){
                            pass_flag = true;
                            break;
                        }
                    }
                }
                if(detail_flag){
                    printf("search KeyArray[%d]=%d  ---->  ", i, KeyArray[i]);
                    btree_node_print(sear);
                }
            }
            if(pass_flag == false){
                print_int_array(KeyArray, len_array);  // 打印当前数组
                printf("following node DOT has KeyArray[%d]=%d!\n", i, KeyArray[i]);
                btree_node_print(sear);
                pass_flag = false;
                break;
            }
        }
        if(pass_flag){
            if(detail_flag) printf("PASS---->查找测试%d/%d\n", i_test+1, max_test);
        }else{
            printf("FAIL---->查找测试%d/%d\n", i_test+1, max_test);
            break;
        }

        if(detail_flag){
            printf("-------------------B树删除测试------------------\n");
        }
        /*-------------------B树删除测试------------------*/
        for(int i=0; i<len_array; i++){
            // if(i==496){
            //     // 加一句打印方便调试暂停
            //     printf("Now delete KeyArray[%d]=%d:\n", i, KeyArray[i]);
            // }
            if(i>0){
                btree_delete_key(T_old, KeyArray[i-1]);
            }
            btree_delete_key(T, KeyArray[i]);
            if(detail_flag){
                printf("delete KeyArray[%d]=%d:\n", i, KeyArray[i]);
                btree_traversal(T);
            } 
            if(btree_check_effective(T) == false){
                print_int_array(KeyArray, len_array);  // 打印当前数组
                printf("after delete KeyArray[%d]=%d error!\n", i, KeyArray[i]);
                pass_flag = false;
                break;
            }
        }
        if(pass_flag){
            if(detail_flag) printf("PASS---->删除测试%d/%d\n", i_test+1, max_test);
        }else{
            printf("FAIL---->删除测试%d/%d\n", i_test+1, max_test);
            // printf("Before delete:\n");
            // btree_traversal(T_old);
            // printf("After delete:\n");
            // btree_traversal(T);

            btree_destroy(T_old);
            btree_destroy(T);
            break;
        }


        if(detail_flag){
            printf("--------------------------------------------------\n");
        }

        btree_destroy(T_old);
        btree_destroy(T);

        // 整点进度条看看
        if(pass_flag){
            // printf("PASS----> WHOLE TEST %d/%d!\r", i_test+1, max_test);
            int bar_process;         // 编译器初始化为0
            bool already_print_txt;  // 编译器初始化为false
            bool already_print_bar;  // 编译器初始化为false
            const int len_bar = 20;  // 完整进度条的长度
            // 打印进度条前面的说明
            if(!already_print_txt){
                printf("PASS TEST PROCESS: ");
                fflush(stdout);
            }
            already_print_txt = true;
            // 打印进度条
            if(len_bar*(i_test+1)/max_test > bar_process){
                // 光标往前回退
                if(already_print_bar){
                    printf("\033[4D");  // ANSI转义序列
                }
                // 画出进度条
                for(int i=0; i<(len_bar*(i_test+1)/max_test - bar_process); i++){
                    printf("█");
                    fflush(stdout);
                }
                // 显示进度范围
                printf(" %d%%", 100*(i_test+1)/max_test);
                fflush(stdout);
                already_print_bar = true;
                bar_process = len_bar*(i_test+1)/max_test;
                if(i_test+1 == max_test) printf("\n");
            }
        }
    }
    // 只是为了最后一行判断用
    if(pass_flag){
        // printf("\r\033[K");  // 清除本行
        printf("PASS----> ALL %d TEST!\n", max_test);
    }
    printf("--------------------------------------------------\n");


    printf("---------------------性能测试---------------------\n");
    btree* bT = btree_init(16);  // 初始化16阶B树
    // 定义时间结构体
    struct timeval tv_begin;
    struct timeval tv_end;
    // 插入性能测试
    gettimeofday(&tv_begin, NULL);
    for(int i=0; i<continue_test_len; i++){
        btree_insert_key(bT, i+1);
    }
    gettimeofday(&tv_end, NULL);
    int time_ms = TIME_SUB_MS(tv_end, tv_begin);
    float qps = (float)continue_test_len / (float)time_ms * 1000;
    printf("total INSERTs:%d  time_used:%d(ms)  qps:%.2f(INSERTs/sec)\n", continue_test_len, time_ms, qps);
    // 查找性能测试
    gettimeofday(&tv_begin, NULL);
    for(int i=0; i<continue_test_len; i++){
        btree_node* node = btree_search_key(bT, i+1);
        int idx = 0;
        for(idx=0; idx<node->num; idx++){
            if(node->keys[idx] == i+1){
                break;
            }
        }
        if(idx == node->num){
            printf("continue_search error!\n");
            return 0;
        }
    }
    gettimeofday(&tv_end, NULL);
    time_ms = TIME_SUB_MS(tv_end, tv_begin);
    qps = (float)continue_test_len / (float)time_ms * 1000;
    printf("total SEARCHs:%d  time_used:%d(ms)  qps:%.2f(SEARCHs/sec)\n", continue_test_len, time_ms, qps);
    // // 删除性能测试
    // gettimeofday(&tv_begin, NULL);
    // for(int i=0; i<continue_test_len; i++){
    //     btree_delete_key(bT, i+1);
    // }
    // gettimeofday(&tv_end, NULL);
    // time_ms = TIME_SUB_MS(tv_end, tv_begin);
    // qps = (float)continue_test_len / (float)time_ms * 1000;
    // printf("total DELETEs:%d  time_used:%d(ms)  qps:%.2f(DELETEs/sec)\n", continue_test_len, time_ms, qps);
    // 销毁B树
    btree_destroy(bT);
    printf("--------------------------------------------------\n");
    return 0;
}

#endif