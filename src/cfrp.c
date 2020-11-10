#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfrp.h"


#define CFRP_ERR -1
#define CFER_SUCC 1
/**
 * 设置非阻塞IO
*/
extern int setnoblocking(int fd){
    int flag = -1;
    if((flag = fcntl(fd, F_GETFL, 0)) < 0){
        return CFRP_ERR;
    }else if(fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0){
        return CFRP_ERR;
    }
    return CFER_SUCC;
}


/**
 * 打包
 * 
*/
extern char* cfrp_pack(cfrp_head *head){
    int size = sizeof(cfrp_head);
    
    char* _pack = (char *)malloc(sizeof(char) * size);
    bzero(_pack, sizeof(char) * size);
    memcpy(_pack, head, size);
    return _pack;
}


/**
 * 解包
*/
extern cfrp_head* cfrp_unpack(char* data){
    int size = sizeof(cfrp_head);
    cfrp_head* _pack = (cfrp_head *)malloc(size);
    bzero(_pack, size);
    memcpy(_pack, data, sizeof(char) * size);
    return _pack;
}


unsigned int cfrp_mask(unsigned int m, unsigned int n, unsigned int b){
    if(m < 0)  m = 0;
    if(b == MASK_1){
        // 11000000 00000000 00000000 00000000
        return (0 << 31 | n > 3 || n == 3 ? 3 << 30 : n << 30) | m << 2 >> 2;
    }else if(b == MASK_2){
        // 00111111 00000000 00000000 000000000 
        return (n > ~(~0 << 6) ? ~(~0 << 6) : n) << 24 | m;
    }else if (b == MASK_3)
    {
       // 00000000 11111111 11111111 11111111
       return m >> 24 << 24 | ( n > ~(~0 << 24) ? ~(~0 << 24): n );
    }
    return m;
}

/**
 * 创建一个cfrp
 * @param peers 
 *        0: 监听地址
 *        1: 转发地址
 * @param type 表示cfrp类型 
*/
extern cfrp* make_cfrp(c_peer peers[], c_cfrp type){

}

/**
 * 启动服务
*/
extern int  cfrp_run(cfrp* _cfrp){

}

/**
 * 停止服务
*/
extern int  cfrp_stop(cfrp* c_frp){

}

/**
 * 创建一个 tcp服务端
*/
extern int make_tcp(c_peer peer, c_sock *sock){

}

/**
 * 创建一个tcp连接端
*/
extern int make_connect(c_peer peer, c_sock *sock){

}