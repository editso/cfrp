#ifndef __CFRP_H__
#define __CFRP_H__
#include "cmap.h"
#include "cqueue.h"


/**
 * mask pos
*/
#define MASK_1 1
#define MASK_2 2
#define MASK_3 3
/**
 * get mask
*/
#define GMASK1(m) m >> 30u
#define GMASK2(m) m << 2u >> 26u
#define GMASK3(m) m & ~(~0u << 24u)

#define EPOLL_SIZE 10

/**
 * 转发消息一次性最大可发送的大小
*/
#define CFRP_BUFF_SIZE 1024 * 1024

#define MAPPER_SIZE 10

#define CFRP_WAIT 1

#define CFRP_DISCONNECT 2

#define CFRP_STOP 3

#define CFRP_CONN 4

#define CFRP_DISMAPPER 5


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
     * 0x00: 关闭连接
     * 0x01: 常规发送
     * 0x02: 断点续传
     * 0x03: 预留
     * 校验信息长度
     * 00111111 0000000 0000000 0000000
     * 本次服务长度
     * 00000000 0000000 0000000 1111111
     */
    unsigned int mask;
    /**
     * 偏移
    */
    unsigned int offset;
}cfrp_head;

/**
 * 一个连接链接端信息
*/
typedef struct{
    int port;
    char* addr;
} c_peer;

/**
 * 当socket 连接成功时包含的信息
*/
typedef struct
{   
    int sfd;
    /**
     * 在每次发送数据包时,发送的数据将被记住
    */
    cqueue mem;
    c_peer peer;
} c_sock;



/**
 * 接收状态
*/
typedef struct{
    int op; 
    /**
     * 0
    */
    cfrp_head head;
    /**
     * 1
    */
    char* order;
    /**
     * 2
    */
    char* data;
}cfrp_state;

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
    /**
     * server or client
    */
    c_cfrp type;
    /**
     * 所有映射信息
    */
    cmap mappers;
    /**
     * 由于网络原因,可能接收到的数据会出现不完整
    */
    cqueue cache;
    /**
     * 当前所在状态
    */
    cfrp_state state;
    /**
     * epoll
     * */ 
    int efd;
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
extern int make_tcp(c_peer *peer, c_sock *sock);

/**
 * 创建一个tcp连接端
*/
extern int make_connect(c_peer *peer, c_sock *sock);

extern cfrp_head* make_head();

extern unsigned int cfrp_mask(unsigned int m, unsigned int n, unsigned int b);

extern char* cfrp_order(unsigned int max);
#endif