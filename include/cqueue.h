#ifndef __CQUEUE_H__
#define __CQUEUE_H__


typedef struct{
    int size;
    int capacity;
    int _lp;
    int _cp;
    int (*_recycle)(void *elem);
    void** _elems;
}cqueue;


/**
 * 创建一个cqueue
*/
cqueue* make_queue(int size, int (*recycle)(void* el));

/**
 * 向队列中添加一个元素
*/
int queue_push(cqueue* queue, void* elem);

/**
 * 弹出一个元素
*/
void* queue_pop(cqueue* queue);

/**
 * 是否是空的队列
*/
int queue_empty(cqueue* queue);


#endif