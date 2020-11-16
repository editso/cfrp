#ifndef __CBUFF_H__
#define __CBUFF_H__


typedef struct{
    int size;
    char* _buf;
}cbuff;


extern int buff_append(cbuff* buff);
extern int buff_insert(cbuff* buff, int offset);
extern char buff_get(cbuff* buff, int index);
extern char* buff_str(cbuff* buff);
#endif