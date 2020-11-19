#include <stdlib.h>
#include <string.h>

#include "clist.h"
#include "clib.h"


extern cnode* make_node(void* el);

extern cnode* get_node(clist* list, int index);

extern cnode* get_node(clist* list, int index){
    if(index < 0 ||  index >= list->size) return (void *)0;
    int i = 0;
    for(cnode* node = list->head; node ; node = node->next, i++){
        if(i == index) return node;
    }
    return (void *)0;
}

extern cnode* make_node(void* el){
    cnode* node = malloc(sizeof(cnode));
    cbzero(node,  sizeof(cnode));
    node->el = el;
    return node;
}

extern clist* make_list(){
    clist* list = malloc(sizeof(clist));
    cbzero(list, sizeof(clist));
    return list;
}

extern int list_add(clist* list, void* el){
    if(! el) return 0;
    cnode* node = make_node(el);
    // 如果头为空说明是第一次插入,直接插入到头
    if(! list->head){
        list->head = node;
    }else{
        // 否则直接添加到尾
        list->tail->next = node;
        node->prev = list->tail;
    }
    // 更新尾
    list->tail = node;
    list->size++;
    return 1;
}


extern void* list_remove(clist* list, int index){
    cnode* node = get_node(list, index);
    if(! node )
        return (void*)0;
    // 被删除的上一个
    cnode* prev = node->prev;
    // 如果有
    if(prev){
        prev->next = node->next;
    }else{
        // 没有说明是头直接更新
        prev = list->head = node->next;
    }
    // 更新被删除的下一个,如果有
    if(prev->next){
        prev->next->prev = prev;
    }
    // 如果删除的的是尾,更新尾巴
    if(list->tail == node)
        list->tail = prev->next;
    void* el = node->el;
    list->size--;
    // 释放空间
    free(node);
    return el;
}


extern void* list_get(clist* list, int index){
    cnode* node  = get_node(list, index);
    return node ? node->el : (void *)0;
}

extern void* list_set(clist* list, int index, void* el){
    cnode* node = get_node(list,  index);
    if(! node )
        return (void *)0;     
    void* _el = node->el;  
    node->el = el;
    return _el;
}


extern int list_insert(clist* list, int index, void* el){
    cnode* node = get_node(list, index);
    // 如果没有直接插入到尾部
    if(! node){
        return list_add(list, el);
    }else{
        // 否则进行插入
        cnode* inode = make_node(el);
        // 更新node的上一个元素对应的下一个指向
        if(node->prev){
            node->prev->next = inode;
        }else{
            // 不如果node->prev没有,则说明是头直接进行更新
            list->head = inode;
        }
        inode->next = node;
        node->prev  =  inode;
        list->size++;
    }
    return 1;
}