# 基于C语言实现内存型数据库（kv存储）

# 源码结构说明

main.c是主函数

# 使用说明

需要准备两台Linux机器，.....。直接输入下面的编译指令：

服务端：
```gcc -o main main.c kvstore.c array.c rbtree.c btree.c hash.c skiplist.c dhash.c```
```./main 9998```

客户端：
```gcc -o tb_kvstore tb_kvstore.c```
```./tb_kvstore 192.168.154.130 9998```

# 后续改进

- 加锁
- 分布式锁
