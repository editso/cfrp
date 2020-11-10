#ifndef __CFRP_H__
#define __CFRP_H__
#include "cmap.h"



#define MASK_1 1
#define MASK_1_0 0
#define MASK_1_2 1 << 30
#define MASK_1_3 2 << 30


#define MASK_2 2

#define MASK_3 3


#define GMASK1(m) m >> 30

#define GMASK2(m) m << 2 >> 26

#define GMASK3(m) m << 8 >> 8


enum{
    CONNECT = 0x00, // 连接
    DISCONNECT = 0x02, // 断开
    EXCEPT = 0x03, // 异常
};

typedef enum{
    SERVER,
    CLIENT
}c_cfrp;

/**
 * 数据包信息
*/
typedef struct{
    /**
     * 类型
     * 1100000 0000000 0000000 0000000
     * 0x01 
     * 0x02
     * 0x03
     * 校验信息长度
     * 00111111 0000000 0000000 0000000
     * 本次服务唯一长度
     * 00000000 0000000 0000000 1111111
     */
    unsigned int mask;
    /**
     * 数据包
    */
    unsigned int size;
}cfrp_head;

/**
 * 一个连接链接端信息
*/
typedef struct{
    const int port;
    const char* addr;
} c_peer;

/**
 * 当socket 连接成功时包含的信息
*/
typedef struct
{
    int fd;
    c_peer peer;
} c_sock;

/**
 * 映射信息
*/
typedef struct{
    /**
     * 
    */
    c_peer peers[2]; 
    /**
     * 0: 映射端
     * 1: 连接端 
     * 2: 服务端
    */
    c_sock sock[3]; 
}cfrp;


/**
 * 设置非阻塞IO
*/
extern int setnoblocking(int fd);

/**
 * 打包
 * 
*/
extern char* cfrp_pack(cfrp_head*);

/**
 * 解包
*/
extern cfrp_head* cfrp_unpack(char*);

/**
 * 创建一个cfrp
 * @param peers 
 *        0: 监听地址
 *        1: 转发地址
 * @param type 表示cfrp类型 
*/
extern cfrp* make_cfrp(c_peer peers[], c_cfrp type);

/**
 * 启动服务
*/
extern int  cfrp_run(cfrp*);
/**
 * 停止服务
*/
extern int  cfrp_stop(cfrp*);

/**
 * 创建一个 tcp服务端
*/
extern int make_tcp(c_peer peer, c_sock *sock);

/**
 * 创建一个tcp连接端
*/
extern int make_connect(c_peer peer, c_sock *sock);


extern unsigned int cfrp_mask(unsigned int m, unsigned int n, unsigned int b);


#endif