#include "kvstore.hpp"

RbTree g_tree;

// ② 左旋：x 的右孩子 y 提到 x 的位置
static void rbtree_left_rotate(RbTree* T, RbNode* x) {
    RbNode* y = x->right;
    x->right = y->left;
    if (y->left != T->nil) y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == T->nil)  // 判断根节点
        T->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

// ③ 右旋：y 的左孩子 x 提到 y 的位置
static void rbtree_right_rotate(RbTree* T, RbNode* y) {
    RbNode* x = y->left;
    y->left = x->right;
    if (x->right != T->nil) x->right->parent = y;
    x->parent = y->parent;
    if (y->parent == T->nil)
        T->root = x;
    else if (y == y->parent->right)
        y->parent->right = x;
    else
        y->parent->left = x;
    x->right = y;
    y->parent = x;
}

// ④ 插入后修复：保持红黑树性质
static void rbtree_insert_fixup(RbTree* T, RbNode* z) {
    while (z->parent->color == RB_RED) {
        if (z->parent == z->parent->parent->left) {
            RbNode* y = z->parent->parent->right;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    rbtree_left_rotate(T, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbtree_right_rotate(T, z->parent->parent);
            }
        } else {
            RbNode* y = z->parent->parent->left;
            if (y->color == RB_RED) {
                z->parent->color = RB_BLACK;
                y->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rbtree_right_rotate(T, z);
                }
                z->parent->color = RB_BLACK;
                z->parent->parent->color = RB_RED;
                rbtree_left_rotate(T, z->parent->parent);
            }
        }
    }
    T->root->color = RB_BLACK;
}

// ⑤ 插入：BST 插入 + fixup
static void rbtree_insert(RbTree* T, RbNode* z) {
    RbNode* y = T->nil;
    RbNode* x = T->root;
    while (x != T->nil) {
        y = x;
        int cmp = strcmp(z->key, x->key);
        if (cmp < 0)
            x = x->left;
        else if (cmp > 0)
            x = x->right;
        else
            return;  // key 已存在，不插入
    }
    z->parent = y;
    if (y == T->nil)
        T->root = z;
    else if (strcmp(z->key, y->key) < 0)
        y->left = z;
    else
        y->right = z;
    z->left = T->nil;
    z->right = T->nil;
    z->color = RB_RED;
    rbtree_insert_fixup(T, z);
}

// ⑥ 搜索：BST 查找
static RbNode* rbtree_search(RbTree* T, const char* key) {
    RbNode* node = T->root;
    while (node != T->nil) {
        int cmp = strcmp(key, node->key);
        if (cmp < 0)
            node = node->left;
        else if (cmp > 0)
            node = node->right;
        else
            return node;
    }
    return T->nil;
}

// ⑦ 找后继（删除时用）
static RbNode* rbtree_minimum(RbTree* T, RbNode* x) {
    while (x->left != T->nil) x = x->left;
    return x;
}

// 找 x 的中序后继：右子树的最左节点；如果没有右子树，往上找
static RbNode* rbtree_successor(RbTree* T, RbNode* x) {
    if (x->right != T->nil) {
        return rbtree_minimum(T, x->right);  // 右子树的最左节点
    }
    // 没有右子树：往上找，直到 x 不是父节点的右孩子
    RbNode* y = x->parent;
    while ((y != T->nil) && (x == y->right)) {
        x = y;
        y = x->parent;
    }
    return y;
}

static void rbtree_delete_fixup(RbTree* T, RbNode* x) {
    // x 是"代替被删节点"的那个节点
    // 如果 x 是黑色，删了它会破坏黑高平衡，需要修复
    while ((x != T->root) && (x->color == RB_BLACK)) {
        if (x == x->parent->left) {
            // 情况 A：x 是左孩子，兄弟 w 是右孩子
            RbNode* w = x->parent->right;

            // case 1：兄弟是红色 → 旋转把兄弟变黑
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                rbtree_left_rotate(T, x->parent);
                w = x->parent->right;
            }

            // case 2：兄弟两个侄子都是黑色 → 兄弟染红，往上推
            if ((w->left->color == RB_BLACK) && (w->right->color == RB_BLACK)) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                // case 3：右侄子是黑色 → 先右旋 w
                if (w->right->color == RB_BLACK) {
                    w->left->color = RB_BLACK;
                    w->color = RB_RED;
                    rbtree_right_rotate(T, w);
                    w = x->parent->right;
                }
                // case 4：左侄子是红色 → 左旋父节点
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->right->color = RB_BLACK;
                rbtree_left_rotate(T, x->parent);
                x = T->root;  // 修复完成，跳出循环
            }

        } else {
            // 情况 B：x 是右孩子，和情况 A 对称（左右互换）
            RbNode* w = x->parent->left;

            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                rbtree_right_rotate(T, x->parent);
                w = x->parent->left;
            }

            if ((w->left->color == RB_BLACK) && (w->right->color == RB_BLACK)) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->left->color == RB_BLACK) {
                    w->right->color = RB_BLACK;
                    w->color = RB_RED;
                    rbtree_left_rotate(T, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->left->color = RB_BLACK;
                rbtree_right_rotate(T, x->parent);
                x = T->root;
            }
        }
    }
    x->color = RB_BLACK;  // 根永远是黑色
}

// 删除节点 z，返回被摘掉的节点（调用方负责释放 key/value/node）
static RbNode* rbtree_delete(RbTree* T, RbNode* z) {
    RbNode *y, *x;

    // 第一步：确定 y 是"真正被删的节点"
    if ((z->left == T->nil) || (z->right == T->nil)) {
        y = z;  // z 最多一个子节点，直接删 z
    } else {
        y = rbtree_successor(T, z);  // z 有两个子节点，删后继
    }

    // 第二步：x 是 y 的唯一子节点（或 nil）
    if (y->left != T->nil)
        x = y->left;
    else if (y->right != T->nil)
        x = y->right;
    else
        x = T->nil;

    // 第三步：把 x 挂到 y 的父节点上（摘链）
    x->parent = y->parent;
    if (y->parent == T->nil)
        T->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;

    // 第四步：如果删的是后继（不是 z 本身），把 y 的 key/value 换到 z
    if (y != z) {
        // 交换 key 指针和 value 指针
        char* tmp = z->key;
        z->key = y->key;
        y->key = tmp;
        tmp = z->value;
        z->value = y->value;
        y->value = tmp;
    }

    // 第五步：如果删的是黑色节点，需要修复平衡
    if (y->color == RB_BLACK) {
        rbtree_delete_fixup(T, x);
    }

    return y;  // 返回被删节点，调用方释放内存
}

// ============ KV 引擎接口（和 array/hash 一模一样） ============

int kvstore_rbtree_create(RbTree* tree) {
    if (!tree) return -1;
    tree->nil = new RbNode;
    tree->nil->color = RB_BLACK;
    tree->nil->left = tree->nil->right = tree->nil->parent = nullptr;
    tree->nil->key = new char[1]();
    tree->root = tree->nil;
    tree->count = 0;
    return 0;
}

void kvstore_rbtree_destroy(RbTree* tree) {
    if (!tree) return;

    // 循环找最小值、删除，直到树空
    RbNode* node;
    while ((node = rbtree_minimum(tree, tree->root)) != tree->nil) {
        RbNode* dead = rbtree_delete(tree, node);
        if (dead) {
            delete[] dead->key;
            delete[] dead->value;
            mp_free(&g_rbnode_pool, dead);
        }
    }

    delete[] tree->nil->key;
    delete tree->nil;
    tree->root = tree->nil = nullptr;
    tree->count = 0;
}

int kvs_rbtree_set(RbTree* tree, const char* key, const char* value) {
    // key 已存在就更新 value
    RbNode* exist = rbtree_search(tree, key);
    if (exist != tree->nil) {
        delete[] exist->value;
        exist->value = new char[strlen(value) + 1]();
        strcpy(exist->value, value);
        return 0;
    }
    // 不存在就新建节点插入
    RbNode* node = (RbNode*)mp_alloc(&g_rbnode_pool);
    node->key = new char[strlen(key) + 1]();
    node->value = new char[strlen(value) + 1]();
    strcpy(node->key, key);
    strcpy(node->value, value);
    rbtree_insert(tree, node);
    tree->count++;
    return 0;
}

const char* kvs_rbtree_get(RbTree* tree, const char* key) {
    RbNode* node = rbtree_search(tree, key);
    if (node == tree->nil) return nullptr;
    return node->value;
}

int kvs_rbtree_delete(RbTree* tree, const char* key) {
    RbNode* node = rbtree_search(tree, key);
    if (node == tree->nil) return tree->count;  // 找不到返回 >0

    RbNode* dead = rbtree_delete(tree, node);  // 调用完整删除
    if (dead) {
        delete[] dead->key;
        delete[] dead->value;
        mp_free(&g_rbnode_pool, dead);
    }
    tree->count--;
    return 0;
}

int kvs_rbtree_modify(RbTree* tree, const char* key, const char* value) {
    RbNode* node = rbtree_search(tree, key);
    if (node == tree->nil) return tree->count;
    delete[] node->value;
    node->value = new char[strlen(value) + 1]();
    strcpy(node->value, value);
    return 0;
}

int kvs_rbtree_count(RbTree* tree) {
    if (!tree) return -1;
    return tree->count;
}
