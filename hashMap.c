#include "hashMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "stack.h"

extern int scope, type, preType;
void yyerror(const char *s);

// Hash Function
unsigned int RSHash(const char *str, unsigned int len)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    unsigned int i = 0;
    for (i = 0; i < len; str++, i++)
    {
        hash = hash * a + (*str);
        a = a * b;
    }
    return hash;
}

HashMap *createHashMap(int size)
{
    HashMap *hashMap = (HashMap *)malloc(sizeof(HashMap));
    hashMap->size = size;
    hashMap->hash_table = (HashNode **)malloc(sizeof(HashNode *) * size);
    int i;
    // init hashMap
    for (i = 0; i < size; i++)
    {
        hashMap->hash_table[i] = NULL;
    }
    return hashMap;
}

// 【关键修复】 variable to data
Data *toData(int type, char *str, struct Declator *declator, int scope)
{
    Data *data = (Data *)malloc(sizeof(Data));
    int len = strlen(str);

    // 修复 1: 必须多分配 1 个字节给 '\0'
    data->id_name = (char *)malloc(len + 1);

    // 修复 2: 使用 strcpy 或者 memcpy 拷贝 len+1 长度
    memcpy(data->id_name, str, len + 1);

    data->type = type;
    data->scope = scope;
    data->declator = declator;
    return data;
}

void freeHashNode(HashNode *hashNode)
{
    Declator *d, *temp;
    if (hashNode->data->declator)
    {
        d = hashNode->data->declator;
        while (d->next)
        {
            temp = d->next;
            free(d);
            d = temp;
        }
        free(d);
    }
    // 释放 id_name 内存
    if (hashNode->data->id_name)
        free(hashNode->data->id_name);
    free(hashNode->data);
    free(hashNode);
}

// 分配内存
void getSize(HashNode *hashNode)
{
    if (hashNode->data->type == STRUCT_TYPE)
    {
        int total = 0;
        hashNode->data->size = total;
    }
    else
    {
        hashNode->data->size = 4;
    }
}

HashNode *createHashNode(Data *data)
{
    HashNode *hashNode = (HashNode *)malloc(sizeof(HashNode));
    hashNode->data = data;
    getSize(hashNode);
    hashNode->next = NULL;
    return hashNode;
}

// add data to hashMap
HashNode *put(HashMap *hashMap, Data *data)
{
    const char *name = data->id_name;
    int pos = RSHash(name, strlen(name)) % hashMap->size;
    HashNode *ptr = hashMap->hash_table[pos];
    if (!ptr)
    {
        hashMap->hash_table[pos] = createHashNode(data);
        return hashMap->hash_table[pos];
    }
    if (ptr->data->scope < data->scope)
    {
        HashNode *hashNode = createHashNode(data);
        hashNode->next = ptr;
        hashMap->hash_table[pos] = hashNode;
        return ptr;
    }
    while (ptr->next && ptr->next->data->scope >= data->scope)
    {
        if (strcmp(ptr->data->id_name, name) == 0 && ptr->data->scope == data->scope)
        {
            return NULL; // already exit;
        }
        ptr = ptr->next;
    }
    if (strcmp(ptr->data->id_name, name) == 0 && ptr->data->scope == data->scope)
    {
        return NULL; // already exit;
    }
    HashNode *hashNode = createHashNode(data);
    hashNode->next = ptr->next;
    ptr->next = hashNode;
    return hashNode;
}

void putTree(HashMap *hashMap, struct Tree *tree)
{
    Stack *s = createStack();
    push(s, tree, 0);
    SNode *n;
    int i;
    /* === 修改 1: 确定真正的类型 === */
    /* 如果全局 type 被清零了(因为遇到了分号)，就使用 preType */
    int currentType = (type == 0) ? preType : type;
    /* =========================== */
    while ((n = pop(s)))
    {
        if (n->tree->num == 0)
        {
            if (strcmp(n->tree->name, "ID"))
            {
                continue;
            }

            // === 修改结束 ===
            if (!put(hashMap, toData(currentType, n->tree->content, n->tree->declator, scope)))
            {
                char *error = (char *)malloc(50);
                sprintf(error, "\"%s\" is  already defined", n->tree->content);
                yyerror(error);
                free(error);
            }
        }
        else
        {
            for (i = 0; i < n->tree->num; i++)
            {
                push(s, n->tree->leaves[i], 0);
            }
        }
    }
}

// find data in hashMap, return NULL when error
HashNode *get(HashMap *hashMap, Data *data)
{
    const char *name = data->id_name;
    int pos = RSHash(name, strlen(name)) % hashMap->size;
    HashNode *ptr = hashMap->hash_table[pos];
    while (ptr)
    {
        if (!ptr->data)
        {
            break;
        }
        // strcmp 依赖 id_name 有 \0 结束符，修复后这里才能正常工作
        if (strcmp(ptr->data->id_name, name) == 0 && ptr->data->scope <= data->scope)
        {
            return ptr;
        }
        ptr = ptr->next;
    }
    return NULL;
}

void printData(HashMap *hashMap)
{
    HashNode *ptr;
    Declator *d;
    int i;
    for (i = 0; i < hashMap->size; i++)
    {
        ptr = hashMap->hash_table[i];
        while (ptr && ptr->data)
        {
            printf("%s %d ", ptr->data->id_name, ptr->data->type);
            for (d = ptr->data->declator; d; d = d->next)
            {
                printf("%d ", d->type);
            }
            printf("%p", ptr->data->adress);
            printf("\n");
            ptr = ptr->next;
        }
    }
}

void destoryPartOfHashMap(HashMap *hashMap, int scope)
{
    int i;
    HashNode *ptr;
    for (i = 0; i < hashMap->size; i++)
    {
        ptr = hashMap->hash_table[i];
        if (!ptr || ptr->data->scope < scope)
        {
            continue;
        }
        if (ptr->data->scope == scope)
        {
            while (ptr && ptr->data->scope == scope)
            {
                hashMap->hash_table[i] = hashMap->hash_table[i]->next;
                freeHashNode(ptr);
                ptr = hashMap->hash_table[i];
            }
            return;
        }
        for (; ptr->next; ptr = ptr->next)
        {
            if (ptr->next->data->scope <= scope)
            {
                break;
            }
        }
        if (!ptr->next)
        {
            return;
        }
        HashNode *before = ptr;
        ptr = ptr->next;
        while (ptr && ptr->data->scope == scope)
        {
            before->next = ptr->next;
            free(ptr);
            ptr = before->next;
        }
    }
}

void destoryHashMap(HashMap *hashMap)
{
    int i;
    HashNode *ptr;
    for (i = 0; i < hashMap->size; i++)
    {
        while ((ptr = hashMap->hash_table[i]))
        {
            hashMap->hash_table[i] = hashMap->hash_table[i]->next;
            freeHashNode(ptr);
        }
    }
    free(hashMap->hash_table);
    free(hashMap);
}

void printHashMap(HashMap *hashMap)
{
    HashNode *ptr;
    int i;
    for (i = 0; i < hashMap->size; i++)
    {
        ptr = hashMap->hash_table[i];
        while (ptr && ptr->data)
        {
            printf("%-10s\t", ptr->data->id_name);
            ptr = ptr->next;
        }
        printf("\n");
    }
}