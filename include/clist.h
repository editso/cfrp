#ifndef __CLIST_H__
#define __CLIST_H__


typedef struct _node{
    void* el;
    struct _node* prev;
    struct _node* next;
}cnode;

typedef struct{
    int size;
    cnode* head;
    cnode* tail;
}clist;


extern clist* make_list();

extern int list_add(clist* list, void* el);

extern void* list_remove(clist* list, int index);

extern void* list_get(clist* list, int index);

extern void* list_set(clist* list, int index, void* el);

extern int list_insert(clist* list, int index, void* el);

#endif