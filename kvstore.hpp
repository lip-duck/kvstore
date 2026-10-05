#ifndef __KVSTORE_HPP__  // ① include guard：防止头文件被重复包含
#define __KVSTORE_HPP__
#include <cassert>     //    assert
#include <cstddef>     // ② C++ 版的 C 标准头文件（size_t 等）
#include <cstdio>      //    snprintf / printf
#include <cstring>     //    memset / strlen / strcpy
#include <functional>  //    std::function（模板暂未用，后续演进预留）

#include "kvstore_mp.hpp"
extern MemPool g_rbnode_pool;  // 专门给 RbNode 用的内存池

// ---------------- ③ 宏开关：决定系统编译成什么形态 ----------------
#define BUFFER_LENGTH 512  // 每个连接的收发缓冲区长度（字节）

// 网络层选择：同一套业务代码，换网络模型只改这一个宏
#define NETWORK_EPOLL 0
#define NETWORK_NTYCO 1
#define NETWORK_IOURING 2
#define ENABLE_NETWORK_SELECT NETWORK_NTYCO  // 当前先只用 epoll

// 引擎开关：先只开 array，其余留 0（后续逐个打开）
#define ENABLE_ARRAY_KVENGINE 1
#define ENABLE_HASH_KVENGINE 0
#define ENABLE_RBTREE_KVENGINE 0

// ---------------- ④ 连接对象：每个 TCP 连接对应一个 ----------------
struct ConnItem {
    int fd = -1;                        // 该连接的文件描述符
    char rbuffer[BUFFER_LENGTH] = {0};  // 收缓冲区
    int rlen = 0;                       // 已收到多少字节
    char wbuffer[BUFFER_LENGTH] = {0};  // 发缓冲区
    int wlen = 0;                       // 待发送多少字节

    // ⑤ 回调函数指针：epoll 就绪后调用的处理函数
    using Callback = int (*)(int fd);  // 类型别名：指向"入参 int、返回 int"的函数
    Callback recv_callback = nullptr;  // 读事件回调
    Callback send_callback = nullptr;  // 写事件回调
};

// ---------------- ⑥ array 引擎 ----------------
constexpr int KVS_ARRAY_SIZE = 1024;  // ⑦ 编译期常量：最多存 1024 对

struct ArrayItem {        // 一对 key/value
    char* key = nullptr;  // 堆上分配的字符串（长度不固定，所以用指针）
    char* value = nullptr;
};

struct Array {
    ArrayItem* table = nullptr;  // 指向堆上一块连续内存（容量 KVS_ARRAY_SIZE）
    int idx = 0;                 // 当前已用下标（也等于元素个数）
};

extern Array g_array;  // ⑧ 全局唯一引擎实例（定义在 .cpp 里）

// ---------------- ⑨ 引擎接口声明（先只声明 array） ----------------
int kvstore_array_create(Array* arr);    // 初始化：分配 table 内存
void kvstore_array_destroy(Array* arr);  // 释放
int kvs_array_set(Array* arr, const char* key, const char* value);
const char* kvs_array_get(Array* arr, const char* key);  // ⑩ 返回 const char*：只读、零拷贝
int kvs_array_delete(Array* arr, const char* key);
int kvs_array_modify(Array* arr, const char* key, const char* value);
int kvs_array_count(Array* arr);

// hash引擎

constexpr int KVS_HASH_SIZE = 1024;

struct HashItem {
    char* key = nullptr;  // 堆上分配的字符串（长度不固定，所以用指针）
    char* value = nullptr;
    HashItem* next = nullptr;
};

struct Hash {
    HashItem* buckets[KVS_HASH_SIZE] = {nullptr};
    int count = 0;
};

extern Hash g_hash;

int kvstore_hash_create(Hash* h);
void kvstore_hash_destroy(Hash* h);
int kvs_hash_set(Hash* h, const char* key, const char* value);
const char* kvs_hash_get(Hash* h, const char* key);
int kvs_hash_delete(Hash* h, const char* key);
int kvs_hash_modify(Hash* h, const char* key, const char* value);
int kvs_hash_count(Hash* h);

// rbtree引擎
#define RB_RED 1
#define RB_BLACK 0

struct RbNode {
    unsigned char color;
    RbNode* left;
    RbNode* right;
    RbNode* parent;
    char* key;
    char* value;
};

struct RbTree {
    RbNode* root;
    RbNode* nil;  // ① NIL 哨兵节点
    int count;
};

extern RbTree g_tree;

int kvstore_rbtree_create(RbTree* tree);
void kvstore_rbtree_destroy(RbTree* tree);
int kvs_rbtree_set(RbTree* tree, const char* key, const char* value);
const char* kvs_rbtree_get(RbTree* tree, const char* key);
int kvs_rbtree_delete(RbTree* tree, const char* key);
int kvs_rbtree_modify(RbTree* tree, const char* key, const char* value);
int kvs_rbtree_count(RbTree* tree);

// ---------------- skiplist 引擎 ----------------
#define SL_MAX_LEVEL 32

struct SkipNode {
    char* key;
    char* value;
    int level;
    SkipNode** forward;  // forward[0] 是第0层后继，forward[1] 是第1层...
};

struct SkipList {
    SkipNode* head;
    int level;
    int count;
};

extern SkipList g_sl;

int kvstore_skiplist_create(SkipList* sl);
void kvstore_skiplist_destroy(SkipList* sl);
int kvs_skiplist_set(SkipList* sl, const char* key, const char* value);
const char* kvs_skiplist_get(SkipList* sl, const char* key);
int kvs_skiplist_delete(SkipList* sl, const char* key);
int kvs_skiplist_modify(SkipList* sl, const char* key, const char* value);
int kvs_skiplist_count(SkipList* sl);

// ---------------- 网络层 & 协议层 ----------------
int epoll_entry();                    // epoll 主循环入口（第 4 步写）
int kvstore_request(ConnItem* item);  // 协议解析入口（第 3 步写）

// ntyco协程网络层
int ntyco_entry(void);

#endif  // __KVSTORE_HPP__