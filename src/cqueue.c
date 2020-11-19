#include <stdlib.h>
#include <string.h>

#include "cqueue.h"
#include "clib.h"

/**
 * 创建一个固定容量的 queue
*/
cqueue* make_queue(int size, int (*recycle)(void* el)){
    cqueue* queue = malloc(sizeof(cqueue));
    cbzero(queue, sizeof(cqueue));
    queue->capacity = size;
    queue->_elems = malloc(sizeof(void *) * size);
    queue->_recycle = recycle;
    return queue;
}

/**
 * 向队列中添加一个元素
*/
int queue_push(cqueue* cqueue, void* elem){
    if(cqueue->size >= cqueue->capacity){
        void* el = queue_pop(cqueue);
        if(el && cqueue->_recycle){
            cqueue->_recycle(el);
        }        
        cqueue->_elems[cqueue->_lp] = elem;
    }else{
        cqueue->_elems[cqueue->_lp++] = elem;
    }
    if(cqueue->_lp >= cqueue->capacity)
        cqueue->_lp = 0;
    cqueue->size++;
    return 1;
}

/**
 * 弹出一个元素
*/
void* queue_pop(cqueue* cqueue){
    if(cqueue->size <= 0)
        return (void*) 0;
    int index = cqueue->_cp;
    if(++cqueue->_cp >= cqueue->capacity){
        cqueue->_cp = 0; // 重置指针
    }
    cqueue->size--;
    return cqueue->_elems[index];
}

/**
 * 是否是空的队列
*/
int queue_empty(cqueue* queue){
    return queue->size <= 0;
}
