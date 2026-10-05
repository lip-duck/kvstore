#include "kvstore.hpp"

Hash g_hash;

static int hash_func(const char* key) {
    unsigned int h = 0;
    while (*key) {
        h = h * 31 + (unsigned char)(*key++);
    }
    return h % KVS_HASH_SIZE;
}

int kvstore_hash_create(Hash* h) {
    if (!h) return -1;
    // buckets 数组已经在 struct 里默认初始化为 nullptr
    h->count = 0;
    return 0;
}

int kvs_hash_set(Hash* h, const char* key, const char* value) {
    if (!h || !key || !value) {
        return -1;
    }
    int idx = hash_func(key);

    HashItem* item = h->buckets[idx];
    while (item) {
        if (strcmp(item->key, key) == 0) {
            delete[] item->value;
            item->value = new char[strlen(value) + 1]();
            strcpy(item->value, value);
            return 0;
        }
        item = item->next;
    }

    // 不存在，新建节点，头插到桶里
    HashItem* newitem = new HashItem;
    newitem->key = new char[strlen(key) + 1]();
    newitem->value = new char[strlen(value) + 1]();
    strcpy(newitem->key, key);
    strcpy(newitem->value, value);
    newitem->next = h->buckets[idx];  // ⑦ 头插法
    h->buckets[idx] = newitem;
    h->count++;
    return 0;
}

void kvstore_hash_destroy(Hash* h) {
    if (!h) return;
    for (int i = 0; i < KVS_HASH_SIZE; i++) {
        HashItem* item = h->buckets[i];
        while (item) {
            HashItem* next = item->next;
            delete[] item->key;
            delete[] item->value;
            delete item;  // ⑤ HashItem 本身也是 new 出来的
            item = next;
        }
        h->buckets[i] = nullptr;
    }
    h->count = 0;
}

const char* kvs_hash_get(Hash* h, const char* key) {
    if (!h || !key) return nullptr;
    int idx = hash_func(key);
    HashItem* item = h->buckets[idx];
    while (item) {
        if (strcmp(item->key, key) == 0) return item->value;
        item = item->next;
    }
    return nullptr;
}

int kvs_hash_delete(Hash* h, const char* key) {
    if (!h || !key) {
        return -1;
    }
    int idx = hash_func(key);
    HashItem* item = h->buckets[idx];
    HashItem* prev = nullptr;

    while (item) {
        if (strcmp(item->key, key) == 0) {
            if (prev) {
                prev->next = item->next;
            } else {
                h->buckets[idx] = item->next;
            }
            delete[] item->key;
            delete[] item->value;
            delete item;
            h->count--;
            return 0;
        }
        prev = item;
        item = item->next;
    }
    return h->count;
}

int kvs_hash_modify(Hash* h, const char* key, const char* value) {
    if (!h || !key || !value) return -1;
    int idx = hash_func(key);
    HashItem* item = h->buckets[idx];
    while (item) {
        if (strcmp(item->key, key) == 0) {
            delete[] item->value;
            item->value = new char[strlen(value) + 1]();
            strcpy(item->value, value);
            return 0;
        }
        item = item->next;
    }
    return h->count;
}

int kvs_hash_count(Hash* h) {
    if (!h) return -1;
    return h->count;
}