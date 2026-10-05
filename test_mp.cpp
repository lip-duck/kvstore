#include <cstdio>

#include "kvstore_mp.hpp"

int main() {
    MemPool m;
    mp_init(&m, 32);

    void* p1 = mp_alloc(&m);
    void* p2 = mp_alloc(&m);
    void* p3 = mp_alloc(&m);
    printf("p1=%p\np2=%p\np3=%p\n", p1, p2, p3);

    mp_free(&m, p2);          // 释放 p2
    void* p4 = mp_alloc(&m);  // 应该拿回 p2
    printf("p4=%p (应该等于 p2)\n", p4);

    mp_destroy(&m);
    return 0;
}
