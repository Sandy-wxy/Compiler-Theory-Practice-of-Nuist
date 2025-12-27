#include "inner.h"
#include "hashMap.h"
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tree.h"

#define MAX_LENGTH 20
#define MAX_SIZE 100000 // 增加缓冲区大小以防大型程序溢出

struct Node *head = NULL;
extern int inner_count;
extern int line_count;

/* 获取二元运算的中间代码节点 */
Node *getNodeByDoubleVar(char *op, char *var0, char *var1, int count)
{
    Node *p = head;
    while (p)
    {
        // 修复：将 = 改为 == 进行比较
        if (p->num == 2 && p->op && !strcmp(p->var[0], var0) && !strcmp(p->var[1], var1) && !strcmp(p->op, op))
        {
            Node *new_node = (Node *)malloc(sizeof(Node));
            new_node->op = NULL;
            new_node->inner = p->inner;
            return new_node;
        }
        p = p->next;
    }

    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->var = (char **)malloc(sizeof(char *) * 2);
    new_node->inner = (char *)malloc(MAX_LENGTH);
    new_node->num = 2;

    sprintf(new_node->inner, "t%d", inner_count++);

    new_node->var[0] = strdup(var0);
    new_node->var[1] = strdup(var1);
    new_node->op = strdup(op);
    new_node->next = head;
    head = new_node;

    return head;
}

/* 获取一元运算的中间代码节点 */
Node *getNodeBySingleVar(char *op, char *var, int count)
{
    Node *p = head;
    while (p)
    {
        // 修复：将 = 改为 == 进行比较
        if (p->num == 1 && p->op && !strcmp(p->var[0], var) && !strcmp(p->op, op))
        {
            Node *new_node = (Node *)malloc(sizeof(Node));
            new_node->op = NULL;
            new_node->inner = p->inner;
            return new_node;
        }
        p = p->next;
    }

    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->var = (char **)malloc(sizeof(char *) * 1);
    new_node->inner = (char *)malloc(MAX_LENGTH);
    new_node->num = 1;

    sprintf(new_node->inner, "t%d", inner_count++);

    new_node->var[0] = strdup(var);
    new_node->op = strdup(op);
    new_node->next = head;
    head = new_node;

    return head;
}

/* 核心函数：合并多个代码片段字符串 */
char *mergeCode(int num, ...)
{
    char *str = (char *)malloc(MAX_SIZE);
    memset(str, 0, MAX_SIZE); // 确保初始状态为空字符串

    va_list valist;
    va_start(valist, num);
    for (int i = 0; i < num; i++)
    {
        char *temp = va_arg(valist, char *);
        if (temp)
        {
            strcat(str, temp);
        }
    }
    va_end(valist);

    if (strlen(str) == 0)
    {
        free(str);
        return NULL;
    }
    return str;
}

/* 将行号转换为 "1: " 格式的字符串 */
char *lineToString(int number)
{
    char *str = (char *)malloc(MAX_LENGTH);
    sprintf(str, "%d: ", number);
    return str;
}

/* 将数字转为字符串 */
char *toString(int number)
{
    char *str = (char *)malloc(MAX_LENGTH);
    sprintf(str, "%d", number);
    return str;
}

/* 生成函数调用指令 */
char *generateFunctionCall(Node *func, Node *args)
{
    if (!func)
        return NULL;
    char *code = (char *)malloc(2048);
    memset(code, 0, 2048);

    // 处理参数入栈
    if (args && args->var)
    {
        for (int i = 0; i < args->num; i++)
        {
            char arg_line[128];
            sprintf(arg_line, "# arg %s\n", args->var[i]);
            strcat(code, arg_line);
        }
    }

    char call_line[128];
    sprintf(call_line, "# call %s\n", func->inner);
    strcat(code, call_line);
    return code;
}

/* 计算数组地址偏移 */
char *calcArrayAddr(Tree *node, char *base_addr)
{
    if (!node || !node->declator || node->declator->type != ARRAY)
        return NULL;

    char *code = (char *)malloc(MAX_SIZE / 10);
    memset(code, 0, MAX_SIZE / 10);
    char *current_base = strdup(base_addr);
    Declator *d = node->declator;

    while (d && d->type == ARRAY)
    {
        char t_offset[24], t_addr[24];
        char *index_var = (d->length && d->length->inner) ? d->length->inner : "0";

        // t_offset = index * 4
        sprintf(t_offset, "t%d", inner_count++);
        char line1[128];
        sprintf(line1, "# %s = %s * 4\n", t_offset, index_var);
        strcat(code, line1);

        // t_addr = current_base + t_offset
        sprintf(t_addr, "t%d", inner_count++);
        char line2[128];
        sprintf(line2, "# %s = %s + %s\n", t_addr, current_base, t_offset);
        strcat(code, line2);

        free(current_base);
        current_base = strdup(t_addr);
        d = d->next;
    }

    node->inner = current_base;
    return code;
}

/* 字符串占位符替换函数：用于在最后统一填充行号 */
int swap(char *text, char *a, char *b)
{
    if (!text || !a || !b)
        return 0;
    char *pos = strstr(text, a);
    if (pos != NULL)
    {
        char back[MAX_SIZE];
        strcpy(back, pos + strlen(a));
        *pos = '\0';
        strcat(text, b);
        strcat(text, back);
        return 1;
    }
    return 0;
}