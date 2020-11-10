#ifndef __MBER_H__
#define __MBER_H__

struct mber
{
    char* key;
    unsigned int len;
    char* data;
};


void dump(void *source, char *data);


void load(char* data, struct mber *mber);
#endif