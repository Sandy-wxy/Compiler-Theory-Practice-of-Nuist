#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "tree.h"
#include "hashMap.h" // 必须包含，否则编译器不认识 HashMap
// 优化选项标志
#define OPT_CONSTANT_FOLDING 1
#define OPT_COMMON_SUBEXPR 2
#define OPT_DEAD_CODE 4
#define TYPE_INT 262
#define TYPE_FLOAT 300
#define TYPE_UNKNOWN 0

// 获取节点的类型
int getExpressionType(Tree *tree, HashMap *map);

// 常量折叠：计算常量表达式
Tree *constantFolding(Tree *tree);

// 公共子表达式消除
Tree *eliminateCommonSubexpr(Tree *tree);

// 死代码删除
Tree *deadCodeElimination(Tree *tree);

// 执行所有优化
Tree *optimizeTree(Tree *tree, int opt_flags);

// 检查节点是否为常量
int isConstant(Tree *tree);

// 计算常量表达式的值
float evaluateConstant(Tree *tree, char *op, float left, float right);
#endif
