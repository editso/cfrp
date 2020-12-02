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
 * state
*/
#define GMASK1(m) m >> 30u
/**
 * token
*/
#define GMASK2(m) m << 2u >> 26u
/**
 * data length
*/
#define GMASK3(m) m & ~(~0u << 24u)


#define EPOLL_SIZE 10

/**
 * 转发消息一次性最大可发送的大小
*/
#define CFRP_BUFF_SIZE 1024 * 1024


/**
 * 错误
*/
#define CFRP_ERR -1
/**
 * 成功
*/
#define CFRP_SUCC 1

/**
 * 映射数量
*/
#define MAPPER_SIZE 10

/**
 * 等待
*/
#define CFRP_WAIT 1

/**
 * 断开
*/
#define CFRP_DISCONNECT 2
/**
 * 停止
*/
#define CFRP_STOP 3
/**
 * 连接
*/
#define CFRP_CONN 4
/**
 * 关闭映射
*/
#define CFRP_DISMAPPER 5


typedef enum{
    SERVER,
    CLIENT
}c_cfrp;

enum{
    CFRP_MESSAGE = 0x01,
    CFRP_POINT = 0x02,
    CFRP_CLOSE = 0x03
};

/**
 * 数据包信息
*/
typedef struct{
    /**
     * 11000000 00000000 00000000 00000000
     * 0x00: 关闭连接
     * 0x01: 常规发送
     * 0x02: 断点续传
     * 0x03: 预留
     * 00111111 00000000 00000000 00000000
     * 校验信息长度
     * 00000000 11111111 11111111 11111111
     * 本次传输数据总长度
     */
    unsigned int mask;
}cfrp_head;

/**
 * 主机地址信息
*/
typedef struct{
    /**
     * 端口号
    */
    int port;
    /**
     * 地址
    */
    char* addr;
} c_peer;


/**
 * cfrp token
*/
typedef struct{
    /**
     * 长度
    */
    unsigned int len;
    /**
     * token信息
    */
    char *token;
}cfrp_token;

/**
 * 当socket 连接成功时包含的信息
*/
typedef struct
{   
    int sfd;
    cfrp_token tok;
    c_peer peer;
} c_sock;


/**
 * 在传输过程中, 由于网络原因客户端发过来的数据不会一次性被接收全
 * 该结构体就是为了保证网络原因然数据不会出现错误
 * 与 cfrp_recv 配和
 * 数据接收状态
*/
typedef struct{
    /**
     * 操作数
     * 1: 已经获得请求头
     * 2: 已经获得请求token
    */
    int op; 
    /**
     * 请求头信息
    */
    cfrp_head head;
    /**
     * 请求附带的token信息
    */
    cfrp_token tok;
}cfrp_state;


/**
 * 映射信息
*/
typedef struct{
    /**
     * epoll
     * */ 
    int efd;
    /**
     * 主机信息
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
    cmap mappings;
    /**
     * 由于网络原因,可能接收到的数据会出现不完整
    */
    cqueue cache;
    /**
     * 当前所在状态
    */
    cfrp_state state;
}cfrp;


/**
 * 由于网络原因客户端发送来过的数据可能是分段的
 * 使用该方法,会一直等待满足大小时才会返回数据
*/
extern int cfrp_recv(cfrp* frp, int fd, char* buff, int size);

/**
 * 创建一个token
*/
extern cfrp_token* make_cfrp_token(cfrp_token* tok, unsigned int len, char* token);

/**
 * 设置非阻塞IO
*/
extern int setnoblocking(int fd);

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

/**
 * 请求头信息
*/
extern int cfrp_mask(int *_mask, unsigned int n, unsigned int b);

/**
 * 生成一个token
*/
extern int cfrp_gentok(char* dest, unsigned int len);
#endif