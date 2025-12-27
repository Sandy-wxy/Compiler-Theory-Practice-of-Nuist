#include "tree.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "stack.h"
#include "hashMap.h"
#include "inner.h"
#include <math.h>

extern char *yytext;
extern int yylineno;
extern FILE *yyout, *out;
extern struct HashMap *hashMap;
extern int scope, type, preType;
void destoryPartOfHashMap(struct HashMap *hashMap, int scope);

extern struct Tree *root;
extern struct Node *head;
int inner_count = 1;
extern int line_count;

// --- 辅助函数声明 ---
Tree *initTree(int num);

// --- 核心函数实现 ---

Tree *createTree(char *name, int number, ...)
{
    int i;
    va_list valist;
    va_start(valist, number);
    if (number == 1)
        return va_arg(valist, Tree *);

    Tree *tree = initTree(1);
    if (!tree)
    {
        printf("Out of space \n");
        exit(0);
    }

    tree->num = number;
    int len = strlen(name);
    tree->name = (char *)malloc(len + 1);
    memcpy(tree->name, name, len + 1);

    char *str = (char *)malloc(1000);
    str[0] = '\0';

    tree->leaves = (Tree **)malloc(sizeof(Tree *) * number);
    for (i = 0; i < number; i++)
    {
        tree->leaves[i] = va_arg(valist, Tree *);
        if (tree->leaves[i]->code)
            strcat(str, tree->leaves[i]->code);
    }
    if (strlen(str) > 0)
        tree->code = str;
    else
        free(str);

    return tree;
}

Tree *terminator(char *name, int yylineno)
{
    Tree *tree = initTree(1);
    if (!tree)
    {
        printf("Out of space \n");
        exit(0);
    }

    tree->num = 0;
    int len = strlen(name);
    tree->name = (char *)malloc(len + 1);
    memcpy(tree->name, name, len + 1);

    tree->line = yylineno;
    if (!strcmp(name, "null"))
        len = 1;
    else
    {
        len = strlen(yytext);
        fprintf(yyout, "%-15s\t%-15s", name, yytext);
        if (!strcmp(name, "ID"))
            fprintf(yyout, "%p\n", hashMap);
        else
        {
            if (type != 0)
            {
                preType = type;
                type = 0;
            }
            if (!strcmp(name, "INT8") || !strcmp(name, "INT10") || !strcmp(name, "INT16"))
                fprintf(yyout, "%s\n", yytext);
            else
            {
                fprintf(yyout, "\n");
                if (!strcmp(name, "COMMA"))
                    type = preType;
            }
        }
        if (!strcmp(name, "LCB"))
        {
            scope++;
            preType = 0;
        }
        else if (!strcmp(name, "RCB"))
            destoryPartOfHashMap(hashMap, scope--);
    }
    tree->content = (char *)malloc(len + 1);
    memcpy(tree->content, yytext, len + 1);
    tree->inner = (char *)malloc(len + 1);
    memcpy(tree->inner, yytext, len + 1);

    return tree;
}

Tree *op(char *name, int num, ...)
{
    Tree *t = initTree(1);
    int len = strlen(name);
    t->name = (char *)malloc(len + 1);
    memcpy(t->name, name, len + 1);
    t->num = num;
    t->leaves = (Tree **)malloc(sizeof(Tree *) * num);
    va_list valist;
    va_start(valist, num);
    Tree *temp = va_arg(valist, Tree *);
    len = strlen(temp->content);
    t->content = (char *)malloc(len + 1);
    memcpy(t->content, temp->content, len + 1);
    int i;
    for (i = 0; i < num; i++)
    {
        temp = va_arg(valist, Tree *);
        t->leaves[i] = temp;
    }
    t->declator = NULL;
    return t;
}

// 二元运算处理：包含类型传播逻辑
Tree *binaryOpr(char *name, Tree *t1, Tree *t2, Tree *t3)
{
    Tree *t = op(name, 2, t2, t1, t3);

    /* --- 修改开始：类型传播逻辑 --- */
    // 默认类型为 INT (262)
    int type1 = t1->type;
    int type2 = t3->type;

    // 如果任一操作数是 FLOAT (300)，结果提升为 FLOAT
    if (type1 == 300 || type2 == 300)
    {
        t->type = 300;
    }
    else
    {
        t->type = 262; // 纯整数运算结果为 INT
    }
    /* --- 修改结束 --- */

    Node *node = getNodeByDoubleVar(t->content, t1->inner, t3->inner, inner_count++);
    t->inner = node->inner;

    if (node->op)
    {
        t->code = mergeCode(11,
                            (t1->code ? t1->code : ""),
                            (t3->code ? t3->code : ""),
                            "#", t->inner, " = ", t1->inner, " ", t->content, " ", t3->inner, "\n");
        line_count++;
    }
    return t;
}

// 赋值运算处理：包含类型检查逻辑
Tree *assignOpr(char *name, Tree *t1, Tree *t2, Tree *t3)
{
    Tree *t = op(name, 2, t2, t1, t3);
    t->inner = t1->inner;
    /* --- 修改开始：类型检查逻辑 --- */

    // t1 是左值 (LHS)，t3 是右值 (RHS)
    // 检查是否将 FLOAT 赋值给 INT (精度丢失)
    if (t1->type == 262 && t3->type == 300)
    {
        printf("\033[33mWarning at line %d: Implicit conversion from FLOAT to INT. Precision loss may occur.\033[0m\n", line_count);
    }
    // 检查完全不兼容的类型 (例如 struct 赋值给 int，假设 struct 类型码是 3)
    else if (t1->type != 0 && t3->type != 0 && t1->type != t3->type)
    {
        // 允许 Int -> Float 的隐式转换，只对其他不匹配报错
        if (!(t1->type == 300 && t3->type == 262))
        {
            printf("Warning at line %d: Type mismatch. Assigning type %d to variable of type %d.\n",
                   line_count, t3->type, t1->type);
        }
    }

    // 赋值表达式的结果类型通常等同于左值类型
    t->type = t1->type;
    /* --- 修改结束 --- */

    t->code = mergeCode(8, t3->code,
                        "#", t1->inner, " ", t->content, " ", t3->inner, "\n");
    line_count++;
    return t;
}

Tree *unaryOpr(char *name, Tree *t1, Tree *t2)
{
    Tree *t = op(name, 1, t1, t2);
    t->inner = mergeCode(2, "t", toString(inner_count++));
    t->code = mergeCode(8, t2->code, "#", t->inner, " = ", t->content, " ", t2->inner, "\n");
    line_count++;
    return t;
}

Tree *ifOpr(char *name, int headline, int nextline, Tree *op, Tree *stmt)
{
    Tree *t = createTree(name, 2, op, stmt);
    t->code = mergeCode(12, op->code,
                        "#", "if ", op->inner, " goto ", toString(headline + 2), "\n",
                        "#", "goto ", toString(nextline), "\n",
                        stmt->code);
    return t;
}

Tree *ifelseOpr(char *name, int headline, int next1, int next2, Tree *op, Tree *stmt1, Tree *stmt2)
{
    Tree *t = createTree(name, 3, op, stmt1, stmt2);
    t->code = mergeCode(17, op->code,
                        "#", "if ", op->inner, " goto ", toString(headline + 2), "\n",
                        "#", "goto ", toString(next1 + 1), "\n",
                        stmt1->code,
                        "#", "goto ", toString(next2), "\n",
                        stmt2->code);
    return t;
}

Tree *whileOpr(char *name, int head1, int head2, int nextline, Tree *op, Tree *stmt)
{
    Tree *t = createTree(name, 2, op, stmt);
    t->code = mergeCode(16, op->code,
                        "#", "if ", op->inner, " goto ", toString(head2 + 2), "\n",
                        "#", "goto ", toString(nextline + 1), "\n",
                        stmt->code,
                        "#", "goto ", toString(head1), "\n");
    return t;
}

Tree *forOpr(char *name, int head1, int head2, int nextline, Tree *op1, Tree *op2, Tree *op3, Tree *stmt)
{
    Tree *t = createTree(name, 3, op1, op2, op3, stmt);
    t->code = mergeCode(18, op1->code,
                        op2->code,
                        "#", "if ", op2->inner, " goto ", toString(head2 + 2), "\n",
                        "#", "goto ", toString(nextline + 1), "\n",
                        stmt->code,
                        op3->code,
                        "#", "goto ", toString(head1), "\n");
    return t;
}

Tree *retNull(char *name, Tree *ret)
{
    Tree *t = createTree(name, 1, ret);
    t->code = "#return\n";
    line_count++;
    return t;
}

Tree *retOpr(char *name, Tree *ret, Tree *op)
{
    Tree *t = createTree(name, 2, ret, op);
    t->code = mergeCode(4, op->code, "#return ", op->inner, "\n");
    line_count++;
    return t;
}

Tree *unaryFunc(char *name, Tree *func, Tree *op)
{
    Tree *t = createTree(name, 2, func, op);
    t->code = mergeCode(6, op->code, "#arg ", op->inner, "\n#call ", name, "\n");
    return t;
}

Tree *addDeclator(char *name, Tree *t1, Tree *t2)
{
    Declator *d;
    if (t1->declator)
    {
        for (d = t1->declator; d->next; d = d->next)
            ;
        d->next = (Declator *)malloc(sizeof(Declator));
        d = d->next;
    }
    else
    {
        t1->declator = (Declator *)malloc(sizeof(Declator));
        d = t1->declator;
    }
    if (!strcmp(name, "Array"))
    {
        d->type = ARRAY;
        t1->declator->length = t2;
    }
    else
    {
        d->type = POINTER;
    }
    d->next = NULL;
    return t1;
}

void printTree(Tree *tree)
{
    int i;
    if (!tree)
        return;
    Stack *s = createStack();
    push(s, tree, 0);
    SNode *temp = NULL;
    while ((temp = pop(s)))
    {
        for (i = 0; i < temp->level; i++)
            fprintf(out, "    ");
        fprintf(out, "%s", temp->tree->name);
        if (temp->tree->content)
            fprintf(out, " %s", temp->tree->content);
        fprintf(out, "\n");
        for (i = temp->tree->num - 1; i >= 0; i--)
            push(s, temp->tree->leaves[i], temp->level + 1);
    }
}

void freeTree(Tree *tree)
{
    if (!tree)
        return;
}

void exportDot(Tree *node, FILE *fp)
{
    if (!node)
        return;
    fprintf(fp, "  n%p [label=\"%s", node, node->name);
    if (node->content)
        fprintf(fp, "\\n%s", node->content);
    fprintf(fp, "\"];\n");

    for (int i = 0; i < node->num; i++)
    {
        if (node->leaves[i])
        {
            fprintf(fp, "  n%p -> n%p;\n", node, node->leaves[i]);
            exportDot(node->leaves[i], fp);
        }
    }
}

Tree *initTree(int num)
{
    Tree *tree = (Tree *)malloc(sizeof(Tree) * num);
    tree->content = NULL;
    tree->name = NULL;
    tree->line = 0;
    tree->num = 0;
    tree->headline = 0;
    tree->nextline = 0;
    tree->inner = NULL;
    tree->code = NULL;
    tree->leaves = NULL;
    tree->next = NULL;
    tree->declator = NULL;
    tree->type = 0; // 关键：初始化类型为0
    return tree;
}