#include "kvstore.hpp"

// 头文件里 extern 声明的全局引擎，在这里"定义"它（分配内存）
Array g_array;

// 初始化：分配 KVS_ARRAY_SIZE 个 ArrayItem 的连续内存
int kvstore_array_create(Array* arr) {
    if (!arr) return -1;
    arr->table = new ArrayItem[KVS_ARRAY_SIZE]();  // ① () 是值初始化
    arr->idx = 0;
    return 0;
}

// set：追加一对 key/value
int kvs_array_set(Array* arr, const char* key, const char* value) {
    if (!arr || !key || !value) return -1;
    if (arr->idx >= KVS_ARRAY_SIZE) return -1;  // 满了

    // ② 为什么要拷贝？调用方的字符串随时可能被覆盖/释放
    char* kcopy = new char[strlen(key) + 1]();  // +1 留给 '\0'
    char* vcopy = new char[strlen(value) + 1]();
    strcpy(kcopy, key);
    strcpy(vcopy, value);

    arr->table[arr->idx].key = kcopy;
    arr->table[arr->idx].value = vcopy;
    arr->idx++;
    return 0;
}

// 销毁：先释放每一对 key/value，再释放整块 table
void kvstore_array_destroy(Array* arr) {
    if (!arr) return;
    for (int i = 0; i < arr->idx; ++i) {
        delete[] arr->table[i].key;
        delete[] arr->table[i].value;
    }
    delete[] arr->table;
    arr->table = nullptr;
    arr->idx = 0;
}



// get：线性扫描，找到返回 value 指针，找不到返回 nullptr
const char* kvs_array_get(Array* arr, const char* key) {
    if (!arr || !key) return nullptr;
    for (int i = 0; i < arr->idx; ++i) {
        if (arr->table[i].key && strcmp(arr->table[i].key, key) == 0) {
            return arr->table[i].value;  // ③ 零拷贝：直接返回内部指针
        }
    }
    return nullptr;
}

// delete：找到后用"最后一项补位"，保持数组紧凑
int kvs_array_delete(Array* arr, const char* key) {
    if (!arr || !key) return -1;
    for (int i = 0; i < arr->idx; ++i) {
        if (strcmp(arr->table[i].key, key) == 0) {
            delete[] arr->table[i].value;
            delete[] arr->table[i].key;
            // ④ 把最后一项移到被删位置，再收缩下标
            arr->table[i] = arr->table[arr->idx - 1];
            arr->table[arr->idx - 1].key = nullptr;  // 防止重复释放
            arr->table[arr->idx - 1].value = nullptr;
            arr->idx--;
            return 0;
        }
    }
    return arr->idx;  // ⑤ 找不到：返回 >0
}

// modify：找到 key，换掉它的 value
int kvs_array_modify(Array* arr, const char* key, const char* value) {
    if (!arr || !key || !value) return -1;
    for (int i = 0; i < arr->idx; ++i) {
        if (strcmp(arr->table[i].key, key) == 0) {
            delete[] arr->table[i].value;  // 先释放旧 value
            arr->table[i].value = new char[strlen(value) + 1]();
            strcpy(arr->table[i].value, value);
            return 0;
        }
    }
    return arr->idx;  // ⑤ 找不到：返回 >0
}

// count：当前元素个数
int kvs_array_count(Array* arr) {
    if (!arr) return -1;
    return arr->idx;
}
