#ifndef LINKLIST_H
#define LINKLIST_H

#include <stdarg.h>

typedef struct ListNode
{
    char *data;
    struct ListNode *before;
    struct ListNode *next;
} ListNode;

typedef struct LinkList
{
    struct ListNode *head;
    struct ListNode *tail;
} LinkList;

// 初始化链表
LinkList *initLinkList(void);

// 初始化链表节点
ListNode *initListNode(void);

// 设置节点数据（深拷贝字符串）
int setListNodeData(ListNode *node, const char *data);

// 向链表末尾添加一个节点
void append(LinkList *list, ListNode *node);

// 向链表末尾添加多个节点
void appendMany(LinkList *list, int num, ...);

// 连接两个链表
void extend(LinkList *list1, LinkList *list2);

// 连接多个链表
void extendMany(LinkList *list, int num, ...);

// 销毁链表，可指定是否释放节点数据
void freeLinkList(LinkList *list, int free_data);

// 获取链表长度
int getLinkListLength(const LinkList *list);

// 打印链表内容
void printLinkList(const LinkList *list);

#endif