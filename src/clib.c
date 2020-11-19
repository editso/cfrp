#include <string.h>

#include "clib.h"

extern void cbzero(void *s, unsigned long int size){
    memset(s, '\0', size);
}