#include "kvstore.hpp"
#define MAX_TOKENS 128
#define CMD_COUNT (sizeof(commands) / sizeof(commands[0]))  // ⑤ 自动算命令数

typedef int (*CmdHandler)(ConnItem* item, char** tokens, int count);

struct Command {
    const char* name;
    CmdHandler handler;
};

static int cmd_set(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_array_set(&g_array, tokens[1], tokens[2]);
    snprintf(item->wbuffer, BUFFER_LENGTH, res == 0 ? "SUCCESS" : "FAILED");
    return 0;
}

static int cmd_get(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    const char* val = kvs_array_get(&g_array, tokens[1]);
    if (val)
        snprintf(item->wbuffer, BUFFER_LENGTH, "%s", val);
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_del(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_array_delete(&g_array, tokens[1]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_mod(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_array_modify(&g_array, tokens[1], tokens[2]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_count(ConnItem* item, char** tokens, int count) {
    (void)count;
    int cnt = kvs_array_count(&g_array);
    if (cnt < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "%d", cnt);
    return 0;
}

static int cmd_hset(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_hash_set(&g_hash, tokens[1], tokens[2]);
    snprintf(item->wbuffer, BUFFER_LENGTH, res == 0 ? "SUCCESS" : "FAILED");
    return 0;
}

static int cmd_hget(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    const char* val = kvs_hash_get(&g_hash, tokens[1]);
    if (val)
        snprintf(item->wbuffer, BUFFER_LENGTH, "%s", val);
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_hdel(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_hash_delete(&g_hash, tokens[1]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_hmod(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_hash_modify(&g_hash, tokens[1], tokens[2]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_hcount(ConnItem* item, char** tokens, int count) {
    (void)count;
    int cnt = kvs_hash_count(&g_hash);
    if (cnt < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "%d", cnt);
    return 0;
}

static int cmd_rset(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_rbtree_set(&g_tree, tokens[1], tokens[2]);
    snprintf(item->wbuffer, BUFFER_LENGTH, res == 0 ? "SUCCESS" : "FAILED");
    return 0;
}

static int cmd_rget(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    const char* val = kvs_rbtree_get(&g_tree, tokens[1]);
    if (val)
        snprintf(item->wbuffer, BUFFER_LENGTH, "%s", val);
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_rdel(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_rbtree_delete(&g_tree, tokens[1]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_rmod(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_rbtree_modify(&g_tree, tokens[1], tokens[2]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_rcount(ConnItem* item, char** tokens, int count) {
    (void)count;
    int cnt = kvs_rbtree_count(&g_tree);
    if (cnt < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "%d", cnt);
    return 0;
}

static int cmd_sset(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_skiplist_set(&g_sl, tokens[1], tokens[2]);
    snprintf(item->wbuffer, BUFFER_LENGTH, res == 0 ? "SUCCESS" : "FAILED");
    return 0;
}

static int cmd_sget(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    const char* val = kvs_skiplist_get(&g_sl, tokens[1]);
    if (val)
        snprintf(item->wbuffer, BUFFER_LENGTH, "%s", val);
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_sdel(ConnItem* item, char** tokens, int count) {
    if (count < 2) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_skiplist_delete(&g_sl, tokens[1]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_smod(ConnItem* item, char** tokens, int count) {
    if (count < 3) {
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERR ARGS");
        return -1;
    }
    int res = kvs_skiplist_modify(&g_sl, tokens[1], tokens[2]);
    if (res < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else if (res == 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "SUCCESS");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "NO EXIST");
    return 0;
}

static int cmd_scount(ConnItem* item, char** tokens, int count) {
    (void)count;
    int cnt = kvs_skiplist_count(&g_sl);
    if (cnt < 0)
        snprintf(item->wbuffer, BUFFER_LENGTH, "ERROR");
    else
        snprintf(item->wbuffer, BUFFER_LENGTH, "%d", cnt);
    return 0;
}


static Command commands[] = {
    {"SET", cmd_set},
    {"GET", cmd_get},
    {"DEL", cmd_del},
    {"MOD", cmd_mod},
    {"COUNT", cmd_count},

    {"HSET", cmd_hset},
    {"HGET", cmd_hget},
    {"HDEL", cmd_hdel},
    {"HMOD", cmd_hmod},
    {"HCOUNT", cmd_hcount},

    {"RSET", cmd_rset},
    {"RGET", cmd_rget},
    {"RDEL", cmd_rdel},
    {"RMOD", cmd_rmod},
    {"RCOUNT", cmd_rcount},

    {"SSET", cmd_sset},
    {"SGET", cmd_sget},
    {"SDEL", cmd_sdel},
    {"SMOD", cmd_smod},
    {"SCOUNT", cmd_scount},

};

// ③ 分词：把 "SET Name King" 拆成 ["SET", "Name", "King"]
int kvstore_split_token(char* msg, char** tokens) {
    if (!msg || !tokens) return -1;

    int idx = 0;
    char* token = strtok(msg, " ");  // 第一次调用传原串
    while (token) {
        tokens[idx++] = token;
        token = strtok(nullptr, " ");  // 后续传 nullptr 继续
    }
    return idx;
}

// ④ 核心：根据命令编号调用对应引擎函数，结果写入 wbuffer
int kvstore_parser_protocol(ConnItem* item, char** tokens, int count) {
    if (!item || !tokens[0] || count <= 0) return -1;

    for (size_t i = 0; i < CMD_COUNT; i++) {
        if (strcmp(commands[i].name, tokens[0]) == 0) {
            memset(item->wbuffer, 0, BUFFER_LENGTH);
            return commands[i].handler(item, tokens, count);  // ⑧ 直接调函数指针
        }
    }

    snprintf(item->wbuffer, BUFFER_LENGTH, "UNKNOWN CMD");
    return -1;
}

// ⑥ 入口：网络层把 rbuffer 给它，处理完结果就在 wbuffer
int kvstore_request(ConnItem* item) {
    char* msg = item->rbuffer;
    char* tokens[MAX_TOKENS] = {nullptr};  // 栈上数组，每个请求独立
    int count = kvstore_split_token(msg, tokens);
    kvstore_parser_protocol(item, tokens, count);
    return 0;
}

// ⑦ 引擎生命周期
int init_kvengine() {
    kvstore_array_create(&g_array);
    kvstore_hash_create(&g_hash);
    kvstore_rbtree_create(&g_tree);
    kvstore_skiplist_create(&g_sl);
    mp_init(&g_rbnode_pool, sizeof(RbNode));
    return 0;
}

int exit_kvengine() {
    kvstore_array_destroy(&g_array);
    kvstore_hash_destroy(&g_hash);
    kvstore_rbtree_destroy(&g_tree);
    kvstore_skiplist_destroy(&g_sl);    
    return 0;
}

int main() {
    init_kvengine();  // 创建引擎
    ntyco_entry();    
    exit_kvengine();  // 正常不会走到这
    return 0;
}
