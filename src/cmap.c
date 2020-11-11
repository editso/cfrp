#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cmap.h"
#include "clist.h"


extern int get_max_prime(int num);

extern unsigned long map_hash(cmap* map, void* el);


extern cmap_elem* make_map_elem(void* key, void* value);


extern cmap_elem* make_map_elem(void* key, void* value){
    cmap_elem *el = malloc(sizeof(cmap_elem));
    el->key = key;
    el->value = value;
    return el;
}


unsigned long map_hash(cmap* map, void* key){
    return (key ? (unsigned long)key : 0);
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


cmap* make_map(int capacity){
    capacity = capacity < 2 ? 2 : capacity;
    cmap* map = malloc(sizeof(cmap));
    bzero(map, sizeof(cmap));
    map->capacity = capacity;
    map->_elems = malloc( sizeof(clist) * capacity);
    map->_prime = get_max_prime(capacity);
    return map;
}


/**
 * 添加
*/
extern int map_put(cmap* map, void* key, void* value){
    cmap_elem *el = make_map_elem(key, value);
    int hash = map_hash_fun(map, key);
    clist *list = map->_elems[hash];
    if(! list){
        list = map->_elems[hash] = make_list();
    }
    return list_add(list, el);;
}


/**
 * 删除
*/
extern void *map_remove(cmap *map, void* key){

}

/**
 * 获取
*/
extern cmap_elem *map_get(cmap *map, void* key){
    int hash = map_hash(map, key);
    clist *list = map->_elems[hash];
    cmap_elem *el;
    for(cnode *node = list->head; node; node = node->next){
        el = node->el;
        if(map_hash(map,  key) == hash){
            return el;
        }
    }
    return (void*)0;
}

extern void *map_set(cmap *map, void* key, void* value);