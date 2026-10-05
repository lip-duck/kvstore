#include <cstdlib>

#include "kvstore.hpp"

SkipList g_sl;

// 随机生成层数：每层 50% 概率继续往上
static int sl_random_level() {
    int level = 1;
    while (level < SL_MAX_LEVEL && (rand() % 2 == 0)) {
        level++;
    }
    return level;
}

// 创建节点：分配 forward 数组
static SkipNode* sl_create_node(int level, const char* key, const char* value) {
    SkipNode* node = new SkipNode;
    node->level = level;
    node->key = new char[strlen(key) + 1]();
    node->value = new char[strlen(value) + 1]();
    strcpy(node->key, key);
    strcpy(node->value, value);
    node->forward = new SkipNode*[level]();  // 每层一个指针，初始化为 NULL
    return node;
}

// 查找：从最高层往低层扫
static SkipNode* sl_search(SkipList* sl, const char* key) {
    SkipNode* p = sl->head;
    for (int i = sl->level - 1; i >= 0; i--) {
        while (p->forward[i] && strcmp(p->forward[i]->key, key) < 0) {
            p = p->forward[i];
        }
    }
    p = p->forward[0];
    if (p && strcmp(p->key, key) == 0) return p;
    return nullptr;
}

int kvstore_skiplist_create(SkipList* sl) {
    if (!sl) return -1;
    sl->head = sl_create_node(SL_MAX_LEVEL, "", "");
    sl->level = 1;
    sl->count = 0;
    return 0;
}

void kvstore_skiplist_destroy(SkipList* sl) {
    if (!sl || !sl->head) return;
    SkipNode* p = sl->head->forward[0];
    while (p) {
        SkipNode* next = p->forward[0];
        delete[] p->key;
        delete[] p->value;
        delete[] p->forward;
        delete p;
        p = next;
    }
    delete[] sl->head->key;
    delete[] sl->head->value;
    delete[] sl->head->forward;
    delete sl->head;
    sl->head = nullptr;
    sl->count = 0;
}

int kvs_skiplist_set(SkipList* sl, const char* key, const char* value) {
    // key 已存在就更新 value
    SkipNode* exist = sl_search(sl, key);
    if (exist) {
        delete[] exist->value;
        exist->value = new char[strlen(value) + 1]();
        strcpy(exist->value, value);
        return 0;
    }

    // 记录每层的前驱节点
    SkipNode* update[SL_MAX_LEVEL];
    SkipNode* p = sl->head;

    for (int i = sl->level - 1; i >= 0; i--) {
        while (p->forward[i] && strcmp(p->forward[i]->key, key) < 0) {
            p = p->forward[i];
        }
        update[i] = p;  // 记录第 i 层的前驱
    }

    // 随机新节点层数
    int new_level = sl_random_level();

    // 如果新层比当前层高，高层的前驱就是 head
    for (int i = sl->level; i < new_level; i++) {
        update[i] = sl->head;
    }

    SkipNode* node = sl_create_node(new_level, key, value);

    // 每层插入：新节点的 forward 指向前驱的后继，前驱指向新节点
    for (int i = 0; i < new_level; i++) {
        node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = node;
    }

    if (new_level > sl->level) {
        sl->level = new_level;
    }
    sl->count++;
    return 0;
}

const char* kvs_skiplist_get(SkipList* sl, const char* key) {
    SkipNode* node = sl_search(sl, key);
    return node ? node->value : nullptr;
}

int kvs_skiplist_delete(SkipList* sl, const char* key) {
    SkipNode* update[SL_MAX_LEVEL];
    SkipNode* p = sl->head;

    for (int i = sl->level - 1; i >= 0; i--) {
        while (p->forward[i] && strcmp(p->forward[i]->key, key) < 0) {
            p = p->forward[i];
        }
        update[i] = p;
    }

    SkipNode* target = p->forward[0];
    if (!target || strcmp(target->key, key) != 0) return sl->count;

    // 每层把前驱的 forward 指向 target 的后继（跳过 target）
    for (int i = 0; i < sl->level; i++) {
        if (update[i]->forward[i] == target) {
            update[i]->forward[i] = target->forward[i];
        }
    }

    delete[] target->key;
    delete[] target->value;
    delete[] target->forward;
    delete target;
    sl->count--;

    // 如果最高层没节点了，降低层数
    while (sl->level > 1 && !sl->head->forward[sl->level - 1]) {
        sl->level--;
    }
    return 0;
}

int kvs_skiplist_modify(SkipList* sl, const char* key, const char* value) {
    SkipNode* node = sl_search(sl, key);
    if (!node) return sl->count;
    delete[] node->value;
    node->value = new char[strlen(value) + 1]();
    strcpy(node->value, value);
    return 0;
}

int kvs_skiplist_count(SkipList* sl) {
    return sl ? sl->count : -1;
}
