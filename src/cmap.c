#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cmap.h"



extern int get_max_prime(int num);

extern unsigned long map_hash(cmap* map, void* key);

extern int map_hash_fun(cmap* map, void *key);

extern cmap_elem* make_map_elem(void* key, void* value);

extern cmap_elem* make_map_elem(void* key, void* value){
    cmap_elem *el = malloc(sizeof(cmap_elem));
    el->key = key;
    el->value = value;
    return el;
}


unsigned long map_hash(cmap* map, void* key){
    return (key ? (int long)key : 2);
}


int map_hash_fun(cmap* map, void* key){
    return map_hash(map,  key) % map->_prime; 
}


extern int get_max_prime(int num){
    for (int i = num; i > 2; --i) {
        for (int j = 2; j < i && i % j != 0; ++j) {
            if (j == i - 1)
                return i;
        }
    }
    return 2;
}

extern cmap* make_map(int capacity){
    capacity = capacity < 2 ? 2 : capacity;
    cmap* map = malloc(sizeof(cmap));
    map_init(map, capacity);
    return map;
}

extern int map_init(cmap* map, int capacity){
    if(! map)return -1;
    bzero(map, sizeof(cmap));
    map->capacity = capacity;
    map->_elems = malloc( sizeof(clist) * capacity);
    map->_prime = get_max_prime(capacity);
    bzero(map->_elems, sizeof(sizeof(clist) * capacity));
}

/**
 * 添加
*/
extern int map_put(cmap* map, void* key, void* value){
    int index = map_hash_fun(map, key);
    unsigned long hash = map_hash(map, key);
    clist *list = map->_elems[index];
    if(!list) list = map->_elems[index] = make_list();
    cmap_elem *tmp;
    for( cnode *node = list->head; node; node = node->next ){
        tmp = node->el;
        if(map_hash(map, tmp->key) == hash){
            tmp->value = value;
            return 1;
        }
    }
    if(! list_add(list, make_map_elem(key, value))) return -1;
    map->size++;
    return 1;
}


/**
 * 删除
*/
extern void *map_remove(cmap *map, void* key){
    unsigned long hash = map_hash(map, key);
    clist* list = map->_elems[ map_hash_fun(map, key) ];
    if( ! list) return (void*)0;
    cmap_elem* el;
    for(cnode *node = list->head; node ; node = node->next){
        el = node->el;
        if(map_hash(map, el->key) == hash){
            if(node->prev){
                node->prev->next = node->next;
            }else{
                list->tail = node->next;
            }
            if(node->next){
                node->next->prev = node->prev;
            }
            void* value = el->value;
            free(el);
            list->size--;
            map->size--;
            return value;
        }
    }
    return (void *)0;
}

/**
 * 获取
*/
extern void *map_get(cmap *map, void* key){
    unsigned long hash = map_hash(map, key);
    clist *list = map->_elems[ map_hash_fun(map, key) ];
    if( ! list) return (void *)0;
    cmap_elem *el;
    for(cnode *node = list->head; node; node = node->next){
        el = node->el;
        if(map_hash(map,  key) == hash){
            return el->value;
        }
    }
    return (void*)0;
}


extern int map_keys(cmap* map, clist* list){
    if(! list) return 0;
    clist* tmp = (void*)0;
    cmap_elem *el;
    for(int i = 0; i < map->capacity; i++){
        tmp = map->_elems[i];
        if(! tmp) continue;
        for(cnode *node = tmp->head; node; node = node->next){
            el = node->el;
            list_add(list, el->key);
        }
    }
    return 1;
}