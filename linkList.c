#include "linkList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

LinkList *initLinkList(void)
{
    LinkList *list = (LinkList *)malloc(sizeof(LinkList));
    if (list == NULL)
    {
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    return list;
}

ListNode *initListNode(void)
{
    ListNode *node = (ListNode *)malloc(sizeof(ListNode));
    if (node == NULL)
    {
        return NULL;
    }
    node->data = NULL;
    node->before = NULL;
    node->next = NULL;
    return node;
}

int setListNodeData(ListNode *node, const char *data)
{
    if (node == NULL || data == NULL)
    {
        return -1;
    }

    // 如果节点已有数据，先释放
    if (node->data != NULL)
    {
        free(node->data);
    }

    // 深拷贝字符串
    node->data = (char *)malloc(strlen(data) + 1);
    if (node->data == NULL)
    {
        return -1;
    }
    strcpy(node->data, data);
    return 0;
}

void append(LinkList *list, ListNode *node)
{
    if (list == NULL || node == NULL)
    {
        return;
    }

    if (list->head == NULL)
    {
        list->head = list->tail = node;
    }
    else
    {
        list->tail->next = node;
        node->before = list->tail;
        list->tail = node;
    }
}

void appendMany(LinkList *list, int num, ...)
{
    if (list == NULL || num <= 0)
    {
        return;
    }

    va_list valist;
    va_start(valist, num);

    for (int i = 0; i < num; i++)
    {
        ListNode *temp = va_arg(valist, ListNode *);
        append(list, temp);
    }

    va_end(valist);
}

void extend(LinkList *list1, LinkList *list2)
{
    if (list1 == NULL || list2 == NULL || list2->head == NULL)
    {
        return;
    }

    if (list1->head == NULL)
    {
        list1->head = list2->head;
        list1->tail = list2->tail;
    }
    else
    {
        list1->tail->next = list2->head;
        list2->head->before = list1->tail;
        list1->tail = list2->tail;
    }

    // 清空list2，避免重复释放
    list2->head = list2->tail = NULL;
}

void extendMany(LinkList *list, int num, ...)
{
    if (list == NULL || num <= 0)
    {
        return;
    }

    va_list valist;
    va_start(valist, num);

    for (int i = 0; i < num; i++)
    {
        LinkList *temp = va_arg(valist, LinkList *);
        extend(list, temp);
    }

    va_end(valist);
}

void freeLinkList(LinkList *list, int free_data)
{
    if (list == NULL)
    {
        return;
    }

    ListNode *current = list->head;
    while (current != NULL)
    {
        ListNode *next = current->next;

        if (free_data && current->data != NULL)
        {
            free(current->data);
        }

        free(current);
        current = next;
    }

    free(list);
}

int getLinkListLength(const LinkList *list)
{
    if (list == NULL)
    {
        return 0;
    }

    int count = 0;
    ListNode *current = list->head;
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}

void printLinkList(const LinkList *list)
{
    if (list == NULL)
    {
        printf("List is NULL\n");
        return;
    }

    printf("LinkList (length: %d): ", getLinkListLength(list));

    ListNode *current = list->head;
    while (current != NULL)
    {
        if (current->data != NULL)
        {
            printf("\"%s\"", current->data);
        }
        else
        {
            printf("NULL");
        }

        if (current->next != NULL)
        {
            printf(" <-> ");
        }
        current = current->next;
    }
    printf("\n");
}