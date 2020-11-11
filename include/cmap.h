#ifndef __CMAP_H__
#define __CMAP_H__


typedef struct{
    // map大小
    int size;
    // 容量
    int capacity;
    // 最大质数
    int _prime;
    // 元素
    void **_elems;
}cmap;


typedef struct{
    void* key;
    void* value;
}cmap_elem;


extern cmap* make_map(int capacity);

/**
 * 添加
*/
extern int map_put(cmap* map, void* key, void* value);

/**
 * 删除
*/
extern void *map_remove(cmap *map, void* key);

/**
 * 获取
*/
extern cmap_elem *map_get(cmap *map, void* key);

extern void *map_set(cmap *map, void* key, void* value);
#endif