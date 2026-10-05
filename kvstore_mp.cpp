// kvstore_mp.cpp
#include "kvstore_mp.hpp"

MemPool g_rbnode_pool;

int mp_init(MemPool* m, int size) {
    if (!m) return -1;
    if (size < 16) size = 16;  // 块不能太小，至少能存一个指针

    m->block_size = size;

    // ① 一次性申请一整页
    m->mem = (char*)malloc(MEM_PAGE_SIZE);
    if (!m->mem) return -1;

    m->free_ptr = m->mem;
    m->free_count = MEM_PAGE_SIZE / size;  // 这一页能切多少块

    // ② 把所有小块串成链表
    //    每个空闲块的前 8 个字节，存"下一个空闲块"的地址
    char* ptr = m->free_ptr;
    for (int i = 0; i < m->free_count; i++) {
        *(char**)ptr = ptr + size;  // 当前块 → 指向下一块
        ptr += size;
    }
    *(char**)ptr = nullptr;  // 最后一块的 next = NULL

    return 0;
}

void mp_destroy(MemPool* m) {
    if (!m || !m->mem) return;
    free(m->mem);  // 一次性释放整块，不用逐个 free
    m->mem = nullptr;
    m->free_ptr = nullptr;
    m->free_count = 0;
}

void* mp_alloc(MemPool* m) {
    if (!m || m->free_count == 0) return nullptr;

    void* ptr = m->free_ptr;     // ③ 拿走链表头
    m->free_ptr = *(char**)ptr;  //    free_ptr 后移一块
    m->free_count--;
    return ptr;
}

void mp_free(MemPool* m, void* ptr) {
    if (!m || !ptr) return;

    // ④ 把块挂回链表头
    *(char**)ptr = m->free_ptr;  // 被释放的块 → 指向旧的链表头
    m->free_ptr = (char*)ptr;    // 链表头 = 这个块
    m->free_count++;
}
