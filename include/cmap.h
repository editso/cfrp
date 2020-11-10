#ifndef __CMAP_H__
#define __CMAP_H__



typedef struct{
    int size;
    void **elems;
}cmap;

/**
 * 添加
*/
int cmap_put(cmap* map, void* el);

/**
 * 删除
*/
void *cmap_remove(cmap *map, void* key);

/**
 * 获取
*/
void *cmap_get(cmap *map, void* key);
#endif