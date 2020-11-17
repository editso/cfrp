#ifndef __CBUFF_H__
#define __CBUFF_H__


typedef struct{
    /**
     * 长度
    */
    int length;
    /**
     * 容量
    */
    unsigned int _capatity;
    char* _buf;
}cbuff;


extern cbuff* make_buff(unsigned int capatity);

extern cbuff* init_buff(cbuff* buff, int capatity);

extern int buff_append(cbuff* buff, char chr);

extern int buff_appends(cbuff* buff, void* ptr, unsigned int size);

extern char buff_get(cbuff* buff, int index);

extern char* buff_str(cbuff* buff);

extern void* buff_sub(cbuff* buff, void* dest, int begin, int end);

extern int buff_recycle(cbuff* buff);

#endif