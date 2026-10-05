// kvstore_mp.hpp
#ifndef KVSTORE_MP_HPP
#define KVSTORE_MP_HPP

#include <cstdlib>
#include <cstring>

#define MEM_PAGE_SIZE 4096  // 一页 4KB

struct MemPool {
    int block_size;  // 每个块多大
    int free_count;  // 还剩多少块
    char* free_ptr;  // free list 的头指针
    char* mem;       // 整块内存的起始地址（free 时释放）
};

int mp_init(MemPool* m, int size);
void mp_destroy(MemPool* m);
void* mp_alloc(MemPool* m);
void mp_free(MemPool* m, void* ptr);

#endif
