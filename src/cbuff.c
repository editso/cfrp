#include <string.h>
#include <stdlib.h>

#include "cbuff.h"
#include "clib.h"

/**
 * 扩张容量
*/
extern int buff_increase(cbuff* buff, int size);


extern int buff_increase(cbuff* buff, int size){
    if(! buff || buff->length >= size) return 0;
    buff->_buf = realloc(buff->_buf, size);
    buff->_capatity = size;
}

extern cbuff* make_buff(unsigned int capatity){
    cbuff* buff = calloc(1, sizeof(cbuff));
    buff->_capatity = capatity <= 0 ? 1 : capatity;
    buff->_buf = calloc(1, sizeof(char) * buff->_capatity);
    buff->length = 0;
}

extern cbuff* init_buff(cbuff* buff, int capatity){
    if(! buff) return buff;
    cbzero(buff, sizeof(cbuff));
    buff->_capatity = capatity <= 0 ? 1 : capatity;
    buff->_buf = calloc(1, sizeof(char) * buff->_capatity);
}


extern int buff_append(cbuff* buff, char chr){
    return buff_appends(buff, &chr, sizeof(chr));
}

extern int buff_appends(cbuff* buff, void* ptr, unsigned int size){
    if(buff->length >= buff->_capatity){
        buff_increase(buff, buff->_capatity * 2);
    }
    memcpy(buff->_buf + buff->length, ptr, size);
    buff->length += size;
}

extern char buff_get(cbuff* buff, int index){
    if(! buff || buff->length <= index) return '\0';
    return buff->_buf[index];
}

extern void* buff_sub(cbuff* buff, void* dest, int begin, int end){
    if( ! buff  || ! dest || buff->length < end || end < begin) 
        return dest;
    memcpy(dest, buff->_buf + begin, end);
    return dest;
}

extern char* buff_cpy(cbuff* buff){
    if(! buff || buff->length <= 0) return (void *)0;
    char* tmp = calloc(1, sizeof(char) * buff->length);
    memcpy(tmp, buff->_buf, sizeof(char) *  buff->length);
    return tmp;
}


extern int buff_recycle(cbuff* buff){
    free(buff->_buf);
}