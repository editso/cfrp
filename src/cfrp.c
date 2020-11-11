#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>


#include "cfrp.h"


#define CFRP_ERR -1
#define CFER_SUCC 1


#define SOCK_ADDR_IN(peer) \
{\
        .sin_family = AF_INET,\
        .sin_port = htons(peer->port),\
        .sin_addr = {.s_addr = inet_addr(peer->addr)}\
}

#define SOCKADDR(addr) (struct sockaddr*)addr


extern cfrp* mmake_cfrp();
extern cfrp* make_cfrp_client(c_peer peers[]);
extern cfrp* make_cfrp_server(c_peer peers[]);
extern int   make_epoll();
extern int   run_server(cfrp* frp);
extern int   run_client(cfrp* frp);


typedef struct{
    int sfd;
    int *ptr;
}cfrp_epoll_data;


#define CFRP_EPOLL_DATA(sfd, ptr) \
{\
    .sfd = sfd,\
    .ptr = ptr;\
}


#define __DEF_EPOLL_EVENT__(__vname, __events, __data) \
    struct epoll_event __vname;\
    __vname.events = __events;\
    __vname.data.ptr = __data;
    
#define __DEF_EPOLL_WAIT__(__vname, __vename, efd, __count, __max, __timeout) \
    struct epoll_event __vename[__count];\
    int __vname = epoll_wait(efd, __vename,  __max, __timeout);
    

#define EPOLL_ADD(__efd, __sfd, ev) epoll_ctl(__efd, EPOLL_CTL_ADD, __sfd, ev) 


#define CFRP_EPOLL_EVENT EPOLLET|EPOLLIN

#define LISTEN\
    for(;;)

#define HANDLER_EPOLL_EVENT(count, ev, __events) \
    ev = __events[0];\
    for (int i; i < count; ev = __events[i++])


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
        return (0 << 31 | n > 3 || n == 3 ? 3 << 30 : n << 30) | (m & ~(~0 << 30));
    }else if(b == MASK_2){
        // 00111111 00000000 00000000 000000000 
        return (n > ~(~0 << 6) ? ~(~0 << 6) : n) << 24 | GMASK1(m) << 30 | GMASK3(m);
    }else if (b == MASK_3)
    {
       // 00000000 11111111 11111111 11111111
       return ~(0 << 8) << 24 & m | ( n > ~(~0 << 24) ? ~(~0 << 24): n );
    }
    return m;
}


extern cfrp* mmake_cfrp(){
    cfrp* frp = (cfrp*)malloc(sizeof(cfrp));
    bzero(frp, sizeof(cfrp));
    return frp;
}

extern int make_epoll(){
    int efd = epoll_create(EPOLL_SIZE);
    if(efd < 0)return CFRP_ERR;
    return efd;
}

/**
 * server
*/
extern cfrp* make_cfrp_server(c_peer peers[]){
    cfrp* frp = mmake_cfrp();    
    if( make_tcp(peers, frp->sock)             == CFRP_ERR || 
        make_tcp((peers + 1), frp->sock + 1)   == CFRP_ERR ){
        perror("cfrp make error");
        exit(1);
    } 
    if( setnoblocking(frp->sock->sfd)          == CFRP_ERR ||
        setnoblocking((frp->sock + 1)->sfd)    == CFRP_ERR ){
        perror("cfrp set noblocking error");
        exit(1);
    }
    if ( (frp->efd = make_epoll())             ==  CFRP_ERR){
        perror("cfrp epoll error");
        exit(0);
    }
    __DEF_EPOLL_EVENT__(ev, CFRP_EPOLL_EVENT, NULL);
    if(EPOLL_ADD(frp->efd, frp->sock->sfd, &ev)         < 0 ||
       EPOLL_ADD(frp->efd, (frp->sock + 1)->sfd, &ev)   < 0){
       perror("epoll error");
       exit(1); 
    }
    return frp;
}

/**
 * client
*/
extern cfrp* make_cfrp_client(c_peer peers[]){
    cfrp* frp = mmake_cfrp();
    if( make_connect(peers, frp->sock)         == CFRP_ERR ||
        setnoblocking(frp->sock->sfd)          == CFRP_ERR ){
            perror("cfrp make error");
            exit(1);
    }
    return frp;
}

/**
 * 创建一个cfrp
 * @param peers 
 *        0: 监听地址
 *        1: 转发地址
 * @param type 表示cfrp类型 
*/
extern cfrp* make_cfrp(c_peer peers[], c_cfrp type){
    cfrp* frp;
    if(type == SERVER)
        frp = make_cfrp_server(peers);
    else
        frp = make_cfrp_client(peers);
    frp->type = type;
    return frp;
}


extern int run_server(cfrp* frp){
    struct epoll_event ev;
    LISTEN{
        __DEF_EPOLL_WAIT__(c, events, frp->efd, 5, 10, -1);
        HANDLER_EPOLL_EVENT(c, ev, events){
            
        }
    }
}


extern int run_client(cfrp* frp){
    struct epoll_event ev;
    LISTEN{
        __DEF_EPOLL_WAIT__(c, events, frp->efd, 5, 10, -1);
        HANDLER_EPOLL_EVENT(c, ev, events){
            
        }
    }
}

/**
 * 启动服务
*/
extern int  cfrp_run(cfrp* frp){
    int pid = vfork();
    if(pid <  0){
        printf("fork child process err");
        exit(1);
    }else if (pid == 0)
    {
        if(frp->type == SERVER){
            run_server(frp);
        }else{
            run_client(frp);
        }
    }else{
        return pid;
    }
}


/**
 * 停止服务
*/
extern int  cfrp_stop(cfrp* c_frp){

}

/**
 * 创建一个 tcp服务端
*/
extern int make_tcp(c_peer *peer, c_sock *sock){
    int fd = -1;
    int val = 1;
    struct sockaddr_in addr = SOCK_ADDR_IN(peer);
    if( (fd= socket(AF_INET, SOCK_STREAM, 0))                           < 0  ||
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int))     < 0  || 
        bind(fd,  SOCKADDR(&addr), sizeof(struct sockaddr_in))          < 0  ||
        listen(fd, 10)                                                  < 0  ){
        perror("make tcp error");
        return CFRP_ERR;
    }
    sock->sfd = fd;
    sock->peer = peer;
    return CFER_SUCC;
}

/**
 * 创建一个tcp连接端
*/
extern int make_connect(c_peer* peer, c_sock *sock){
    int fd = -1;
    struct sockaddr_in addr = SOCK_ADDR_IN(peer);
    if( (fd = socket(AF_INET, SOCK_STREAM, 0))                < 0 ||
        connect(fd, SOCKADDR(&addr), sizeof(struct sockaddr)) < 0 ){
        return CFRP_ERR;
    }
    sock->sfd = fd;
    sock->peer = peer;
    return fd;
}

extern cfrp_head* make_head(){
    cfrp_head* head  = (cfrp_head*)malloc(sizeof(cfrp_head));
    bzero(head, sizeof(cfrp_head));
    return head;
}