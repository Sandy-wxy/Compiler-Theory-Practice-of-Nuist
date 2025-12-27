#include "optimizer.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* 声明外部函数，用于合并代码字符串 */
extern char *mergeCode(int num, ...);
#include "hashMap.h" // 确保能访问符号表

int getExpressionType(Tree *tree, HashMap *map)
{
    if (!tree)
        return 0;

    /* 1. 终结符处理：直接根据节点名称返回类型 */
    if (tree->name)
    {
        if (!strcmp(tree->name, "INT10") || !strcmp(tree->name, "INT8") || !strcmp(tree->name, "INT16"))
            return 262; // INT 类型代码
        if (!strcmp(tree->name, "FLOAT_CONST"))
            return 300; // FLOAT 类型代码

        if (!strcmp(tree->name, "ID"))
        {
            // 从符号表查询变量定义时的类型
            Data *data = toData(0, tree->content, NULL, 0);
            HashNode *node = get(map, data);
            if (node && node->data)
            {
                return node->data->type;
            }
            return 0;
        }
    }

    /* 2. 递归穿透：处理非终结符节点 */
    if (tree->num > 0)
    {
        // 如果是算术运算表达式 (如 additive_expression)，通常有 3 个子节点 [cite: 39]
        if (tree->num == 3)
        {
            int leftT = getExpressionType(tree->leaves[0], map);
            int rightT = getExpressionType(tree->leaves[2], map);

            // 只要有一侧是 FLOAT (300)，结果就是 FLOAT
            if (leftT == 300 || rightT == 300)
                return 300;
            if (leftT == 262 && rightT == 262)
                return 262;

            // 如果其中一侧是 0，则向上透传另一侧的非零类型
            return (leftT != 0) ? leftT : rightT;
        }

        // 如果是单子节点包装 (如 primary_expression -> constant) [cite: 21]
        // 递归进入第一个子节点继续查找
        return getExpressionType(tree->leaves[0], map);
    }

    return 0;
}
/* 声明外部错误处理函数 */
void yyerror(const char *s);

// --- 1. 基础辅助函数 ---

/* 检查节点是否为常量 */
int isConstant(Tree *tree)
{
    if (!tree || !tree->name)
        return 0;
    /* 匹配 Lexer 生成的所有可能常数类型名 */
    return !strcmp(tree->name, "INT10") ||
           !strcmp(tree->name, "INT8") ||
           !strcmp(tree->name, "INT16") ||
           !strcmp(tree->name, "INT") ||
           !strcmp(tree->name, "FLOAT_CONST");
}

/* 从常量节点提取数值 */
static float getConstantValue(Tree *tree)
{
    if (!tree || !tree->content)
        return 0.0f;
    if (!strcmp(tree->name, "INT8"))
    {
        return (float)strtol(tree->content, NULL, 8);
    }
    else if (!strcmp(tree->name, "INT16"))
    {
        return (float)strtol(tree->content, NULL, 16);
    }
    else
    {
        return (float)atof(tree->content);
    }
}

/* 检查是否为特定的常数值（如 0 或 1），用于代数简化 */
int isSpecificConstant(Tree *tree, float target)
{
    if (!isConstant(tree))
        return 0;
    return fabs(getConstantValue(tree) - target) < 1e-9;
}

/* 创建新的常量节点并清除旧指令 */
static Tree *createConstantNode(float value, int isFloat)
{
    Tree *tree = initTree(0);
    tree->num = 0;
    tree->leaves = NULL;
    char buf[64];

    if (isFloat)
    {
        tree->name = strdup("FLOAT_CONST");
        sprintf(buf, "%f", value);
    }
    else
    {
        tree->name = strdup("INT10");
        sprintf(buf, "%d", (int)value);
    }

    tree->content = strdup(buf);
    tree->inner = strdup(buf);

    /* 关键点：将 code 设为 NULL。
       这样父节点在拼接指令时，旧的运算指令（如 t1 = 10 * 5）就会消失。 */
    tree->code = NULL;

    return tree;
}

// --- 2. 核心计算与优化逻辑 ---

/* 执行具体的常量运算 */
float evaluateConstant(Tree *tree, char *op, float left, float right)
{
    if (!op)
        return left;

    if (!strcmp(op, "+"))
        return left + right;
    if (!strcmp(op, "-"))
        return left - right;
    if (!strcmp(op, "*"))
        return left * right;
    if (!strcmp(op, "/"))
    {
        if (fabs(right) < 1e-9)
        {
            yyerror("Division by zero in constant expression");
            return 0;
        }
        return left / right;
    }
    if (!strcmp(op, "%"))
    {
        int r = (int)right;
        return (float)((int)left % (r == 0 ? 1 : r));
    }
    if (!strcmp(op, "^"))
        return (float)pow(left, right);
    if (!strcmp(op, ">"))
        return (float)(left > right);
    if (!strcmp(op, "<"))
        return (float)(left < right);
    if (!strcmp(op, ">="))
        return (float)(left >= right);
    if (!strcmp(op, "<="))
        return (float)(left <= right);
    if (!strcmp(op, "=="))
        return (float)(fabs(left - right) < 1e-9);
    if (!strcmp(op, "!="))
        return (float)(fabs(left - right) > 1e-9);
    if (!strcmp(op, "&&"))
        return (float)((int)left && (int)right);
    if (!strcmp(op, "||"))
        return (float)((int)left || (int)right);

    return left;
}

/* 常量折叠与代数简化主函数 */
Tree *constantFolding(Tree *tree)
{
    if (!tree)
        return NULL;

    /* 1. 递归处理子节点 */
    for (int i = 0; i < tree->num; i++)
    {
        if (tree->leaves[i])
            tree->leaves[i] = constantFolding(tree->leaves[i]);
    }

    /* 2. 针对二元运算 (leaves[0] op leaves[2]) 的优化 */
    if (tree->num == 3 && tree->leaves[0] && tree->leaves[2])
    {
        Tree *l = tree->leaves[0];
        Tree *op = tree->leaves[1];
        Tree *r = tree->leaves[2];

        /* A. 常量折叠：10 * 5 -> 50 */
        if (isConstant(l) && isConstant(r) && op->content)
        {
            float res = evaluateConstant(tree, op->content, getConstantValue(l), getConstantValue(r));
            int isF = !strcmp(l->name, "FLOAT_CONST") || !strcmp(r->name, "FLOAT_CONST");
            return createConstantNode(res, isF);
        }

        /* B. 代数简化：a + 0 -> a 或 b * 1 -> b */
        if (!strcmp(op->content, "+"))
        {
            if (isSpecificConstant(r, 0))
                return l;
            if (isSpecificConstant(l, 0))
                return r;
        }
        if (!strcmp(op->content, "*"))
        {
            if (isSpecificConstant(r, 1))
                return l;
            if (isSpecificConstant(l, 1))
                return r;
            if (isSpecificConstant(r, 0) || isSpecificConstant(l, 0))
                return createConstantNode(0, 0);
        }

        /* C. 强度削弱示例：a * 2 -> a + a */
        if (!strcmp(op->content, "*") && isSpecificConstant(r, 2))
        {
            op->content = strdup("+");
            tree->leaves[2] = l;
        }
    }

    /* 3. 赋值语句优化：a = t1 且 t1 已折叠为常量 -> 直接生成 a = 52 */
    if (!strcmp(tree->name, "assignment_expression") && tree->num == 3)
    {
        Tree *l = tree->leaves[0];
        Tree *r = tree->leaves[2];
        if (isConstant(r))
        {
            char buf[128];
            sprintf(buf, "# %s = %s\n", l->inner, r->inner);
            if (tree->code)
                free(tree->code);
            tree->code = strdup(buf);
        }
    }

    return tree;
}

/* 死代码删除：剪除不可达的分支 */
Tree *deadCodeElimination(Tree *tree)
{
    if (!tree)
        return NULL;

    for (int i = 0; i < tree->num; i++)
    {
        if (tree->leaves[i])
            tree->leaves[i] = deadCodeElimination(tree->leaves[i]);
    }

    /* 优化 if/else 语句 */
    if (!strcmp(tree->name, "if_expression") || !strcmp(tree->name, "if_else_expression"))
    {
        Tree *cond = tree->leaves[0];
        if (isConstant(cond))
        {
            float val = getConstantValue(cond);
            if (fabs(val) > 1e-9)
            {
                return tree->leaves[2]; /* 条件恒真，保留 then */
            }
            else
            {
                return (tree->num > 5) ? tree->leaves[5] : NULL; /* 条件恒假，保留 else 或删除 */
            }
        }
    }

    /* 优化 while(0) */
    if (!strcmp(tree->name, "while_expression"))
    {
        Tree *cond = (tree->num > 3) ? tree->leaves[3] : NULL;
        if (cond && isConstant(cond) && fabs(getConstantValue(cond)) < 1e-9)
        {
            return NULL;
        }
    }

    return tree;
}

// --- 3. 外部接口 ---

Tree *optimizeTree(Tree *tree, int opt_flags)
{
    if (!tree)
        return NULL;

    if (opt_flags & OPT_CONSTANT_FOLDING)
    {
        tree = constantFolding(tree);
    }

    if (opt_flags & OPT_DEAD_CODE)
    {
        tree = deadCodeElimination(tree);
    }

    return tree;
}

/* 在 tree.c 或 optimizer.c 中添加 */
void propagateType(Tree *parent)
{
    if (!parent || parent->num == 0)
        return;

    // 如果是单子节点包装，直接继承
    if (parent->num == 1)
    {
        parent->type = parent->leaves[0]->type;
    }
    // 如果是二元运算，执行简单的类型提升逻辑
    else if (parent->num == 3)
    {
        int t1 = parent->leaves[0]->type;
        int t2 = parent->leaves[2]->type;
        if (t1 == 300 || t2 == 300)
            parent->type = 300;
        else
            parent->type = 262;
    }
}