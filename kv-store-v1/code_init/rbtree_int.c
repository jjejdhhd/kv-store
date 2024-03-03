#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

// 编译指令：gcc -o main rbtree_int.c
// 本代码实现红黑树，存储int型key，未指定value。

#define RBTREE_DEBUG 1  // 是否运行测试代码

typedef int KEY_TYPE;  // 节点的key类型
#define RED   1
#define BLACK 0

// 定义红黑树单独节点
typedef struct _rbtree_node {
    KEY_TYPE key;      // 键
    void *value;  // 值，可以指向任何类型
    struct _rbtree_node *left;
    struct _rbtree_node *right;
    struct _rbtree_node *parent;
    unsigned char color;  // 不同编译器的无符号性质符号不同，这里加上unsigned减少意外。
    /* 对于32位系统，上述只有color是1个字节，其余都是4个字节，所以color放在最后可以节省内存。 */
} rbtree_node;

// 定义整个红黑树
typedef struct _rbtree{
    struct _rbtree_node *root_node; // 根节点
    struct _rbtree_node *nil_node; // 空节点，也就是叶子节点、根节点的父节点
} rbtree;

// 存储打印红黑树所需的参数
typedef struct _disp_parameters{
    // 打印缓冲区
    char **disp_buffer;
    // 打印缓冲区的深度，宽度，当前打印的列数
    int disp_depth;
    int disp_width;
    int disp_column;
    // 树的深度
    int max_depth;
    // 最大的数字位宽
    int max_num_width;
    // 单个节点的显示宽度
    int node_width;
}disp_parameters;


/*----初始化及释放内存----*/
// 红黑树初始化，注意调用完后释放内存rbtree_free
rbtree *rbtree_malloc(void);
// 红黑树释放内存
void rbtree_free(rbtree *T);

/*----插入操作----*/
// 红黑树插入
void rbtree_insert(rbtree *T, KEY_TYPE key, void *value);
// 调整插入新节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_insert_fixup(rbtree *T, rbtree_node *cur);

/*----删除操作----*/
// 红黑树删除
void rbtree_delete(rbtree *T, rbtree_node *del);
// 调整删除某节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_delete_fixup(rbtree *T, rbtree_node *cur);

/*----查找操作----*/
// 红黑树查找
rbtree_node* rbtree_search(rbtree *T, KEY_TYPE key);

/*----打印信息----*/
// 中序遍历整个红黑树，依次打印节点信息
void rbtree_traversal(rbtree *T);
// 以图的形式展示红黑树
void rbtree_display(rbtree *T);
// 先序遍历，打印红黑树信息到字符数组指针
void set_display_buffer(rbtree *T, rbtree_node *cur, disp_parameters *p);

/*----检查有效性----*/
// 检查当前红黑树的有效性：根节点黑色、红色不相邻、所有路径黑高相同
bool rbtree_check_effective(rbtree *T);

/*----其他函数----*/
// 在给定节点作为根节点的子树中，找出key最小的节点
rbtree_node* rbtree_min(rbtree *T, rbtree_node *cur);
// 在给定节点作为根节点的子树中，找出key最大的节点
rbtree_node* rbtree_max(rbtree *T, rbtree_node *cur);
// 找出当前节点的前驱节点
rbtree_node* rbtree_precursor_node(rbtree *T, rbtree_node *cur);
// 找出当前节点的后继节点
rbtree_node* rbtree_successor_node(rbtree *T, rbtree_node *cur);
// 红黑树节点左旋，无需修改颜色
void rbtree_left_rotate(rbtree *T, rbtree_node *x);
// 红黑树节点右旋，无需修改颜色
void rbtree_right_rotate(rbtree *T, rbtree_node *y);
// 计算红黑树的深度
int rbtree_depth(rbtree *T);
// 递归计算红黑树的深度（不包括叶子节点）
int rbtree_depth_recursion(rbtree *T, rbtree_node *cur);

/*-----------------------------下面为函数定义-------------------------------*/
// 红黑树初始化，注意调用完后释放内存rbtree_free()
rbtree *rbtree_malloc(void){
    rbtree *T = (rbtree*)malloc(sizeof(rbtree));
    if(T == NULL){
        printf("rbtree malloc failed!");
    }else{
        T->nil_node = (rbtree_node*)malloc(sizeof(rbtree_node));
        T->nil_node->color = BLACK;
        T->nil_node->left = T->nil_node;
        T->nil_node->right = T->nil_node;
        T->nil_node->parent = T->nil_node;
        T->root_node = T->nil_node;
    }
    return T;
}

// 红黑树释放内存
void rbtree_free(rbtree *T){
    free(T->nil_node);
    free(T);
}

// 在给定节点作为根节点的子树中，找出key最小的节点
rbtree_node* rbtree_min(rbtree *T, rbtree_node *cur){  
    while(cur->left != T->nil_node){
        cur = cur->left;
    }
    return cur;
}

// 在给定节点作为根节点的子树中，找出key最大的节点
rbtree_node* rbtree_max(rbtree *T, rbtree_node *cur){  
    while(cur->right != T->nil_node){
        cur = cur->right;
    }
    return cur;
}

// 找出当前节点的前驱节点
rbtree_node* rbtree_precursor_node(rbtree *T, rbtree_node *cur){
    // 若当前节点有左孩子，那就直接向下找
    if(cur->left != T->nil_node){
        return rbtree_max(T, cur->left);
    }

    // 若当前节点没有左孩子，那就向上找
    rbtree_node *parent = cur->parent;
    while((parent != T->nil_node) && (cur == parent->left)){
        cur = parent;
        parent = cur->parent;
    }
    return parent;
    // 若返回值为空节点，则说明当前节点就是第一个节点
}

// 找出当前节点的后继节点
rbtree_node* rbtree_successor_node(rbtree *T, rbtree_node *cur){
    // 若当前节点有右孩子，那就直接向下找
    if(cur->right != T->nil_node){
        return rbtree_min(T, cur->right);
    }

    // 若当前节点没有右孩子，那就向上找
    rbtree_node *parent = cur->parent;
    while((parent != T->nil_node) && (cur == parent->right)){
        cur = parent;
        parent = cur->parent;
    }
    return parent;
    // 若返回值为空节点，则说明当前节点就是最后一个节点
}

// 红黑树节点左旋，无需修改颜色
void rbtree_left_rotate(rbtree *T, rbtree_node *x){
    // 传入rbtree*是为了判断节点node的左右子树是否为叶子节点、父节点是否为根节点。
    rbtree_node *y = x->right;
    // 注意红黑树中所有路径都是双向的，两边的指针都要改！
    // 另外，按照如下的修改顺序，无需存储额外的节点。
    x->right = y->left;
    if(y->left != T->nil_node){
        y->left->parent = x;
    }

    y->parent = x->parent;
    if(x->parent == T->nil_node){  // x为根节点
        T->root_node = y;
    }else if(x->parent->left == x){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}


// 红黑树节点右旋，无需修改颜色
void rbtree_right_rotate(rbtree *T, rbtree_node *y){
    rbtree_node *x = y->left;
    
    y->left = x->right;
    if(x->right != T->nil_node){
        x->right->parent = y;
    }

    x->parent = y->parent;
    if(y->parent == T->nil_node){
        T->root_node = x;
    }else if(y->parent->left == y){
        y->parent->left = x;
    }else{
        y->parent->right = x;
    }

    x->right = y;
    y->parent = x;
}

// 调整插入新节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_insert_fixup(rbtree *T, rbtree_node *cur){
    // 父节点是黑色，无需调整。
    // 父节点是红色，则有如下八种情况。
    while(cur->parent->color == RED){
        // 获取叔节点
        rbtree_node *uncle;
        if(cur->parent->parent->left == cur->parent){
            uncle = cur->parent->parent->right;
        }else{
            uncle = cur->parent->parent->left;
        }

        // 若叔节点为红，只需更新颜色(隐含了四种情况)
        // 循环主要在这里起作用
        if(uncle->color == RED){
            // 叔节点为红色：祖父变红/父变黑/叔变黑、祖父节点成新的当前节点。
            if(uncle->color == RED){
                cur->parent->parent->color = RED;
                cur->parent->color = BLACK;
                uncle->color = BLACK;
                cur = cur->parent->parent;
            }
        }
        // 若叔节点为黑，需要变色+旋转(当前节点相当于祖父节点位置包括四种情况:LL/RR/LR/RL)
        // 下面对四种情况进行判断：都是只执行一次
        else{
            if(cur->parent->parent->left == cur->parent){
                // LL：祖父变红/父变黑、祖父右旋。最后的当前节点应该是原来的当前节点。
                if(cur->parent->left == cur){
                    cur->parent->parent->color = RED;
                    cur->parent->color = BLACK;
                    rbtree_right_rotate(T, cur->parent->parent);
                }
                // LR：祖父变红/父变红/当前变黑、父左旋、祖父右旋。最后的当前节点应该是原来的祖父节点/父节点。
                else{
                    cur->parent->parent->color = RED;
                    cur->parent->color = RED;
                    cur->color = BLACK;
                    cur = cur->parent;
                    rbtree_left_rotate(T, cur);
                    rbtree_right_rotate(T, cur->parent->parent);
                }
            }
            else{
                // RL：祖父变红/父变红/当前变黑、父右旋、祖父左旋。最后的当前节点应该是原来的祖父节点/父节点。
                if(cur->parent->left == cur){
                    cur->parent->parent->color = RED;
                    cur->parent->color = RED;
                    cur->color = BLACK;
                    cur = cur->parent;
                    rbtree_right_rotate(T, cur);
                    rbtree_left_rotate(T, cur->parent->parent);
                }
                // RR：祖父变红/父变黑、祖父左旋。最后的当前节点应该是原来的当前节点。
                else{
                    cur->parent->parent->color = RED;
                    cur->parent->color = BLACK;
                    rbtree_left_rotate(T, cur->parent->parent);
                }
            }
        }
    }
    
    // 将根节点变为黑色
    T->root_node->color = BLACK;
}

// 插入
// void rbtree_insert(rbtree *T, rbtree_node *new){
void rbtree_insert(rbtree *T, KEY_TYPE key, void *value){
    // 创建新节点
    rbtree_node *new = (rbtree_node*)malloc(sizeof(rbtree_node));
    new->key = key;
    new->value = value;
    
    // 寻找插入位置（红黑树中序遍历升序）
    rbtree_node *cur = T->root_node;
    rbtree_node *next = T->root_node;
    // 刚插入的位置一定是叶子节点
    while(next != T->nil_node){
        cur = next;
        if(new->key > cur->key){
            next = cur->right;
        }else if(new->key < cur->key){
            next = cur->left;
        }else if(new->key == cur->key){
            // 红黑树本身没有明确如何处理key相同节点，所以取决于业务。
            // 场景1：统计不同课程的人数，相同就+1。
            // 场景2：时间戳，若相同则稍微加一点
            // 其他场景：覆盖、丢弃...
            printf("Already have the same key=%d!\n", new->key);
            free(new);
            return;
        }
    }
    if(cur == T->nil_node){
        // 若红黑树本身没有节点
        T->root_node = new;
    }else if(new->key > cur->key){
        cur->right = new;
    }else{
        cur->left = new;
    }
    new->parent = cur;
    new->left = T->nil_node;
    new->right = T->nil_node;
    new->color = RED;

    // 调整红黑树，使得红色节点不相邻
    rbtree_insert_fixup(T, new);
}


// 调整删除某节点后的红黑树，使得红色节点不相邻(平衡性)
void rbtree_delete_fixup(rbtree *T, rbtree_node *cur){
    // child是黑色、child不是根节点才会进入循环
    while((cur->color == BLACK) && (cur != T->root_node)){
        // 获取兄弟节点
        rbtree_node *brother = T->nil_node;
        if(cur->parent->left == cur){
            brother = cur->parent->right;
        }else{
            brother = cur->parent->left;
        }
        
        // 兄弟节点为红色：父变红/兄弟变黑、父单旋、当前节点下一循环
        if(brother->color == RED){
            cur->parent->color = RED;
            brother->color = BLACK;
            if(cur->parent->left == cur){
                rbtree_left_rotate(T, cur->parent);
            }else{
                rbtree_right_rotate(T, cur->parent);
            }
        }
        // 兄弟节点为黑色
        else{ 
            // 兄弟节点没有红色子节点：父变黑/兄弟变红、看情况是否结束循环
            if((brother->left->color == BLACK) && (brother->right->color == BLACK)){
                // 若父原先为黑，父节点成新的当前节点进入下一循环；否则结束循环。
                if(brother->parent->color == BLACK){
                    cur = cur->parent;
                }else{
                    cur = T->root_node;
                }
                brother->parent->color = BLACK;
                brother->color = RED;
            }
            // 兄弟节点有红色子节点：LL/LR/RR/RL
            else if(brother->parent->left == brother){
                // LL：红子变黑/兄弟变父色/父变黑、父右旋，结束循环
                if(brother->left->color == RED){
                    brother->left->color = BLACK;
                    brother->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_right_rotate(T, brother->parent);
                    cur = T->root_node;
                }
                // LR：红子变父色/父变黑、兄弟左旋/父右旋，结束循环
                else{
                    brother->right->color = brother->parent->color;
                    cur->parent->color = BLACK;
                    rbtree_left_rotate(T, brother);
                    rbtree_right_rotate(T, cur->parent);
                    cur = T->root_node;
                }
            }else{
                // RR：红子变黑/兄弟变父色/父变黑、父左旋，结束循环
                if(brother->right->color == RED){
                    brother->right->color = BLACK;
                    brother->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_left_rotate(T, brother->parent);
                    cur = T->root_node;
                }
                // RL：红子变父色/父变黑、兄弟右旋/父左旋，结束循环
                else{
                    brother->left->color = brother->parent->color;
                    brother->parent->color = BLACK;
                    rbtree_right_rotate(T, brother);
                    rbtree_left_rotate(T, cur->parent);
                    cur = T->root_node;
                }
            }
        }
    }
    // 下面这行处理情况2/3
    cur->color = BLACK;
}

// 红黑树删除
void rbtree_delete(rbtree *T, rbtree_node *del){
    if(del != T->nil_node){
        /* 红黑树删除逻辑：
            1. 标准的BST删除操作(本函数)：最红都会转换成删除只有一个子节点或者没有子节点的节点。
            2. 若删除节点为黑色，则进行调整(rebtre_delete_fixup)。
        */
        rbtree_node *del_r = T->nil_node;        // 实际删除的节点
        rbtree_node *del_r_child = T->nil_node;  // 实际删除节点的子节点

        // 找出实际删除的节点
        // 注：实际删除的节点最多只有一个子节点，或者没有子节点(必然在最后两层中，不包括叶子节点那一层)
        if((del->left == T->nil_node) || (del->right == T->nil_node)){
            // 如果要删除的节点本身就只有一个孩子或者没有孩子，那实际删除的节点就是该节点
            del_r = del;
        }else{
            // 如果要删除的节点有两个孩子，那就使用其后继节点(必然最多只有一个孩子)
            del_r = rbtree_successor_node(T, del);
        }

        // 看看删除节点的孩子是谁，没有孩子就是空节点
        if(del_r->left != T->nil_node){
            del_r_child = del_r->left;
        }else{
            del_r_child = del_r->right;
        }

        // 将实际要删除的节点删除
        del_r_child->parent = del_r->parent;  // 若child为空节点，最后再把父节点指向空节点
        if(del_r->parent == T->nil_node){
            T->root_node = del_r_child;
        }else if(del_r->parent->left == del_r){
            del_r->parent->left = del_r_child;
        }else{
            del_r->parent->right = del_r_child;
        }

        // 替换替换键值对
        if(del != del_r){
            del->key = del_r->key;
            del->value = del_r->value;
        }

        // 最后看是否需要调整
        if(del_r->color == BLACK){
            rbtree_delete_fixup(T, del_r_child);
        }
        
        // 调整空节点的父节点
        if(del_r_child == T->nil_node){
            del_r_child->parent = T->nil_node;
        }
        free(del_r);
    }
}

// 查找
rbtree_node* rbtree_search(rbtree *T, KEY_TYPE key){
    rbtree_node *cur = T->root_node;
    while(cur != T->nil_node){
        if(cur->key > key){
            cur = cur->left;
        }else if(cur->key < key){
            cur = cur->right;
        }else{
            return cur;
        }
    }
    printf("There is NO key=%d in rbtree!\n", key);
    return T->nil_node;
}

// 中序遍历给定结点为根节点的子树（递归）
void rbtree_traversal_node(rbtree *T, rbtree_node *cur){
    if(cur != T->nil_node){
        rbtree_traversal_node(T, cur->left);
        if(cur->color == RED){
            printf("Key:%d\tColor:Red\n", cur->key);
        }else{
            printf("Key:%d\tColor:Black\n", cur->key);
        }
        rbtree_traversal_node(T, cur->right);
    }
}

// 中序遍历整个红黑树
void rbtree_traversal(rbtree *T){
    rbtree_traversal_node(T, T->root_node);
}

// 递归计算红黑树的深度（不包括叶子节点）
int rbtree_depth_recursion(rbtree *T, rbtree_node *cur){
    if(cur == T->nil_node){
        return 0;
    }else{
        int left = rbtree_depth_recursion(T, cur->left);
        int right = rbtree_depth_recursion(T, cur->right);
        return ((left > right) ? left : right) + 1;
    }
}

// 计算红黑树的深度
int rbtree_depth(rbtree *T){
    return rbtree_depth_recursion(T, T->root_node);
}

// 获取输入数字的十进制显示宽度
int decimal_width(int num_in){
    int width = 0;
    while (num_in != 0){
        num_in = num_in / 10;
        width++;
    }
    return width;
}

// 先序遍历，打印红黑树信息到字符数组指针
void set_display_buffer(rbtree *T, rbtree_node *cur, disp_parameters *p){
    if(cur != T->nil_node){
        // 输出当前节点
        p->disp_depth++;
        // 输出数字到缓冲区
        char num_char[20];
        char formatString[20];
        int cur_num_width = decimal_width(cur->key);
        int num_space = (p->node_width - 2 - cur_num_width) >> 1;  // 数字后面需要补充的空格数量
        strncpy(formatString, "|%*d", sizeof(formatString));
        int i = 0;
        for(i=0; i<num_space; i++){
            strncat(formatString, " ", 2);
        }
        strncat(formatString, "|", 2);
        snprintf(num_char, sizeof(num_char), formatString, (p->node_width-2-num_space), cur->key);
        i = 0;
        while(num_char[i] != '\0'){
            p->disp_buffer[(p->disp_depth-1)*3][p->disp_column+i] = num_char[i];
            i++;
        }
        // 输出颜色到缓冲区
        char color_char[20];
        if(cur->color == RED){
            num_space = (p->node_width-2-3)>>1;
            strncpy(color_char, "|", 2);
            for(i=0; i<(p->node_width-2-3-num_space); i++){
                strncat(color_char, " ", 2);
            }
            strncat(color_char, "RED", 4);
            for(i=0; i<num_space; i++){
                strncat(color_char, " ", 2);
            }
            strncat(color_char, "|", 2);
        }else{
            num_space = (p->node_width-2-5)>>1;
            strncpy(color_char, "|", 2);
            for(i=0; i<(p->node_width-2-5-num_space); i++){
                strncat(color_char, " ", 2);
            }
            strncat(color_char, "BLACK", 6);
            for(i=0; i<num_space; i++){
                strncat(color_char, " ", 2);
            }
            strncat(color_char, "|", 2);
        }
        // strcpy(color_char, (cur->color == RED) ? "| RED |" : "|BLACK|");
        i = 0;
        while(color_char[i] != '\0'){
            p->disp_buffer[(p->disp_depth-1)*3+1][p->disp_column+i] = color_char[i];
            i++;
        }
        // 输出连接符到缓冲区
        if(p->disp_depth>1){
            char connector_char[10];
            strcpy(connector_char, (cur->parent->left == cur) ? "/" : "\\");
            p->disp_buffer[(p->disp_depth-1)*3-1][p->disp_column+(p->node_width>>1)] = connector_char[0];
        }

        // 下一层需要前进/后退的字符数
        int steps = 0;
        if(p->disp_depth+1 == p->max_depth){
            steps = (p->node_width>>1)+1;
        }else{
            steps = (1<<(p->max_depth - p->disp_depth - 2)) * p->node_width;
        }

        // 输出左侧节点
        p->disp_column -= steps;
        set_display_buffer(T, cur->left, p);
        p->disp_column += steps;
        
        // 输出右侧节点
        if(p->disp_depth+1 == p->max_depth){
            steps = p->node_width-steps;
        }
        p->disp_column += steps;
        set_display_buffer(T, cur->right, p);
        p->disp_column -= steps;
        
        p->disp_depth--;
    }
}

// 以图的形式展示红黑树
void rbtree_display(rbtree *T){
    // 红黑树为空不画图
    if(T->root_node == T->nil_node){
        printf("rbtree DO NOT have any key!\n");
        return;
    }

    // 初始化参数结构体
    disp_parameters *para = (disp_parameters*)malloc(sizeof(disp_parameters));
    if(para == NULL){
        printf("disp_parameters struct malloc failed!");
        return;
    }
    rbtree_node *max_node = rbtree_max(T, T->root_node);
    para->max_num_width = decimal_width(max_node->key);    
    para->max_depth = rbtree_depth(T);
    para->node_width = (para->max_num_width<=5) ? 7 : (para->max_num_width+2);  // 边框“||”宽度2 + 数字宽度
    para->disp_depth = 0;
    para->disp_width = para->node_width * (1 << (para->max_depth-1)) + 1;
    para->disp_column = ((para->disp_width-para->node_width)>>1);
    int height = (para->max_depth-1)*3 + 2;
    // 根据树的大小申请内存
    para->disp_buffer = (char**)malloc(sizeof(char*)*height);
    int i = 0;
    for(i=0; i<height; i++){
        para->disp_buffer[i] = (char*)malloc(sizeof(char)*para->disp_width);
        memset(para->disp_buffer[i], ' ', para->disp_width);
        para->disp_buffer[i][para->disp_width-1] = '\0';
    }

    // 打印内容
    set_display_buffer(T, T->root_node, para);
    for(i=0; i<height; i++){
        printf("%s\n", para->disp_buffer[i]);
    }

    // 释放内存
    for(i=0; i<height; i++){
        free(para->disp_buffer[i]);
    }
    free(para->disp_buffer);
    free(para);
}


// 检查当前红黑树的有效性：根节点黑色、红色不相邻、所有路径黑高相同
bool rbtree_check_effective(rbtree *T){
    bool rc_flag = true;  // 根节点黑色
    bool rn_flag = true;  // 红色不相邻
    bool bh_flag = true;  // 所有路径黑高相同
    if(T->root_node->color == RED){
        printf("ERROR! root-node's color is RED!\n");
        rc_flag = false;
    }else{
        int depth = rbtree_depth(T);
        int max_index_path = 1<<(depth-1);  // 从根节点出发的路径总数
        // 获取最左侧路径的黑高
        int black_height = 0;
        rbtree_node *cur = T->root_node;
        while(cur != T->nil_node){
            if(cur->color == BLACK) black_height++;
            cur = cur->left;
            // printf("bh = %d\n", black_height);
        }
        // 遍历每一条路径
        int i_path = 0;
        for(i_path=1; i_path<max_index_path; i_path++){
            int dir = i_path;
            int bh = 0;  // 当前路径的黑高
            cur = T->root_node;
            while(cur != T->nil_node){
                // 更新黑高
                if(cur->color == BLACK){
                    bh++;
                }
                // 判断红色节点不相邻
                else{
                    if((cur->left->color == RED) || (cur->right->color == RED)){
                        printf("ERROR! red node %d has red child!\n", cur->key);
                        rn_flag = false;
                    }
                }
                // 更新下一节点
                // 0:left, 1:right
                if(dir%2) cur = cur->right;
                else      cur = cur->left;
                dir = dir>>1;
            }
            if(bh != black_height){
                printf("ERROR! black height is not same! path 0 is %d, path %d is %d.\n", black_height, i_path, bh);
                bh_flag = false;
            }
        }
    }
    return (rc_flag && rn_flag && bh_flag);
}
/*------------------------------------------------------------------------*/


/*-----------------------------下面为测试代码-------------------------------*/
#if RBTREE_DEBUG
#include<time.h>  // 使用随机数
#include<sys/time.h>  // 计算qps中获取时间
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
#define ENABLE_QPS  1  // 是否开启qps性能测试
#define continue_test_len  1000000  // 连续测试的长度
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
int main(){
    /* --------------------定义数组-------------------- */
    // 预定义的数组
    // int KeyArray[20] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};  // 正着插
    // int KeyArray[20] = {20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};  // 倒着插
    // int KeyArray[20] = {1,2,3,4,5,10,9,8,7,6,11,12,13,14,15,20,19,18,17,16};  // 乱序插
    // int KeyArray[31] = {11,12,13,14,15,16,17,18,19,20,1,2,3,4,5,6,7,8,9,10,21,22,23,24,25,26,27,28,29,30,31};  // 乱序插

    // 顺序增长的数组
    int len_array = 18;
    int KeyArray[len_array];
    int i_array = 0;
    for(i_array=0; i_array<len_array; i_array++){
        KeyArray[i_array] = i_array + 1;
    }

    // // 随机生成固定大小的随机数组
    // int len_array = 18;
    // int KeyArray[len_array];
    // srand(time(NULL));
    // int i_array = 0;
    // for(i_array=0; i_array<len_array; i_array++){
    //     KeyArray[i_array] = rand() % 9999999999;
    // }

    /* ------------------以下测试代码------------------ */
    printf("-------------------红黑树插入测试------------------\n");
    // 先给输入的数组排个序
    int len_max = sizeof(KeyArray)/sizeof(int);
    printf("测试数组长度: %d\n", len_max);
    // 将原先的数组深拷贝并升序排序
    int *KeyArray_sort = (int*)malloc(sizeof(KeyArray));
    printf("RAND_MAX: %d\n", RAND_MAX);
    int i = 0;
    for(i = 0; i < len_max; i++){
        KeyArray_sort[i] = KeyArray[i];
    }
    bubble_sort(KeyArray_sort, len_max);

    // 申请红黑树内存
    rbtree *T = rbtree_malloc();

    // 依次插入数据
    for(i = 0; i < len_max; i++){
        rbtree_insert(T, KeyArray[i], NULL);
    }

    // 遍历数据，查看是否符合红黑树性质
    // rbtree_display(T);
    if(rbtree_check_effective(T)){
        printf("PASS---->插入测试\n");
    }else{
        printf("FAIL---->插入测试\n");
    }
    // rbtree_display(T);

    printf("-------------------红黑树前驱节点测试------------------\n");
    int pass_flag = 1;
    if(rbtree_precursor_node(T, rbtree_search(T, KeyArray_sort[0])) != T->nil_node){
        printf("search first key %d's precursor error! get %d, expected nil_node\n", len_max, rbtree_precursor_node(T, rbtree_search(T, KeyArray_sort[0]))->key);
        pass_flag = 0;
    }
    for(i = 1; i<len_max; i++){
        rbtree_node *precursor = rbtree_precursor_node(T, rbtree_search(T, KeyArray_sort[i]));
        if(precursor->key != KeyArray_sort[i-1]){
            printf("search key %d error! get %d, expected %d\n", KeyArray_sort[i], precursor->key, KeyArray_sort[i-1]);
            pass_flag = 0;
        }
    }
    if(pass_flag){
        printf("PASS---->前驱节点测试\n");
    }else{
        printf("FAIL---->前驱节点测试\n");
    }

    printf("-------------------红黑树后继节点测试------------------\n");
    pass_flag = 1;
    if(rbtree_successor_node(T, rbtree_search(T, KeyArray_sort[len_max-1])) != T->nil_node){
        printf("search last key %d's successor error! get %d, expected nil_node\n",\
                KeyArray_sort[len_max-1],\
                rbtree_successor_node(T, rbtree_search(T, KeyArray_sort[len_max-1]))->key);
        pass_flag = 0;
    }
    for(i = 0; i<len_max-1; i++){
        rbtree_node *successor = rbtree_successor_node(T, rbtree_search(T, KeyArray_sort[i]));
        if(successor->key != KeyArray_sort[i+1]){
            printf("search key %d error! get %d, expected %d\n", KeyArray_sort[i], successor->key, KeyArray_sort[i+1]);
            pass_flag = 0;
        }
    }
    if(pass_flag){
        printf("PASS---->后继节点测试\n");
    }else{
        printf("FAIL---->后继节点测试\n");
    }

    printf("-------------------红黑树删除测试------------------\n");
    // 依次删除所有元素
    for(i=0; i<len_max; i++){
        rbtree_delete(T, rbtree_search(T, KeyArray_sort[i]));
        if(!rbtree_check_effective(T)){
            rbtree_display(T);
            printf("FAIL---->删除测试%d\n", i+1);
            break;
        }else{
            printf("PASS---->删除测试%d\n", i+1);
        }
    }

    printf("-------------------红黑树打印测试------------------\n");
    // 先插入数据1~18，再删除16/17/18，即可得到4层的满二叉树
    for(i = 0; i < len_max; i++){
        rbtree_insert(T, KeyArray[i], NULL);
    }
    for(i=0; i<3; i++){
        rbtree_delete(T, rbtree_search(T, KeyArray_sort[len_max-i-1]));
        if(!rbtree_check_effective(T)){
            printf("FAIL---->删除测试%d\n", KeyArray_sort[len_max-i-1]);
            break;
        }else{
            printf("PASS---->删除测试%d\n", KeyArray_sort[len_max-i-1]);
        }
    }
    // 打印看看结果
    rbtree_display(T);
    // 清空红黑树
    for(i=0; i<len_max; i++){
        rbtree_delete(T, rbtree_search(T, KeyArray_sort[i]));
    }

#if ENABLE_QPS
    printf("---------------红黑树连续插入性能测试---------------\n");
    // 定义时间结构体
    struct timeval tv_begin;
    struct timeval tv_end;
    gettimeofday(&tv_begin, NULL);
    for(i = 0; i < continue_test_len; i++){
        rbtree_insert(T, i+1, NULL);
    }
    gettimeofday(&tv_end, NULL);
    int time_ms = TIME_SUB_MS(tv_end, tv_begin);
    float qps = (float)continue_test_len / (float)time_ms * 1000;
    printf("total INSERTs:%d  time_used:%d(ms)  qps:%.2f(INSERTs/sec)\n", continue_test_len, time_ms, qps);

    printf("---------------红黑树连续查找性能测试---------------\n");
    gettimeofday(&tv_begin, NULL);
    for(i = 0; i < continue_test_len; i++){
        // rbtree_search(T, i+1);
        if(rbtree_search(T, i+1)->key != i+1){
            printf("continue_search error!\n");
            return 0;
        }
    }
    gettimeofday(&tv_end, NULL);
    time_ms = TIME_SUB_MS(tv_end, tv_begin);
    qps = (float)continue_test_len / (float)time_ms * 1000;
    printf("total SEARCHs:%d  time_used:%d(ms)  qps:%.2f(SEARCHs/sec)\n", continue_test_len, time_ms, qps);

    printf("---------------红黑树连续删除性能测试---------------\n");
    gettimeofday(&tv_begin, NULL);
    for(i = 0; i < continue_test_len; i++){
        rbtree_delete(T, rbtree_search(T, i+1));
    }
    gettimeofday(&tv_end, NULL);
    time_ms = TIME_SUB_MS(tv_end, tv_begin);
    qps = (float)continue_test_len / (float)time_ms * 1000;
    printf("total DELETEs:%d  time_used:%d(ms)  qps:%.2f(DELETEs/sec)\n", continue_test_len, time_ms, qps);
#endif

    printf("--------------------------------------------------\n");
    rbtree_free(T); // 别忘了释放内存
    free(KeyArray_sort);
    return 0;
}
#endif