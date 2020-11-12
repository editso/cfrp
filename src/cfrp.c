#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <time.h>

#include "cfrp.h"
#include "logger.h"

static const char __BASE_LATTER__[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z'
};

/**
 * 创建cfrp
*/
extern cfrp* mmake_cfrp();
/**
 * cfrp连接端
*/
extern cfrp* make_cfrp_client(c_peer peers[]);
/**
 * cfrp服务端
*/
extern cfrp* make_cfrp_server(c_peer peers[]);
/**
 * 多路复用
*/
extern int make_epoll();
/**
 * 启动服务端
*/
extern int run_server(cfrp* frp);
/**
 * 启动客户端
*/
extern int run_client(cfrp* frp);
/**
 * 监听socket
*/
extern int cfrp_listen(cfrp* frp, int sfd, int op, int events, void* data);
/**
 * 允许一个连接
*/
extern int cfrp_accept(int fd, c_sock* sock);
/**
 * 关闭
*/
extern int cfrp_close(cfrp* frp, int fd);
/**
 * 转发接收到的信息
*/
extern int cfrp_recv_forward(cfrp* frp);

/**
 * 转发发送的信息
*/
extern int cfrp_send_forward(cfrp* frp, char *uuid);

/**
 * 注册监听
*/
extern char* cfrp_register(cfrp* frp, c_sock* sock);
/**
 * 取消注册
*/
extern char* cfrp_unregister(cfrp* frp, c_sock* sock);
extern char* cfrp_clear_mappers(cfrp* frp);


#define CFRP_ERR -1
#define CFER_SUCC 1


#define SOCK_ADDR_IN(peer) \
{\
        .sin_family = AF_INET,\
        .sin_port = htons(peer->port),\
        .sin_addr = {.s_addr = inet_addr(peer->addr)}\
}

#define SOCK_PEER(__addr, peer)\
    peer.port = ntohs((__addr)->sin_port);\
    peer.addr = inet_ntoa((__addr)->sin_addr);\

#define SOCKADDR(addr) (struct sockaddr*)addr


typedef struct{
    int sfd;
    void *ptr;
}cfrp_epoll_data;


#define CFRP_EPOLL_EVENT EPOLLET|EPOLLIN

#define __DEF_EPOLL_EVENT__(__vname, __events, __data) \
    struct epoll_event __vname;\
    __vname.events = __events;\
    __vname.data.ptr = __data;
    
#define __DEF_EPOLL_WAIT__(__vname, efd, __events,__max, __timeout) \
    if(efd == -1) break;\
    int __vname = epoll_wait(efd, __events,  __max, __timeout);
    
#define EPOLL_ADD(__frp, __sfd, __data) cfrp_listen(__frp, __sfd, EPOLL_CTL_ADD, CFRP_EPOLL_EVENT, __data)

#define EPOLL_MOD(__frp, __sfd, __ev) epoll_ctl(__frp->efd, EPOLL_CTL_MOD, __sfd, __ev)

#define EPOLL_DEL(__frp, __sfd, __ev) epoll_ctl(__frp->efd, EPOLL_CTL_DEL, __sfd, __ev)

#define LISTEN\
    for(;;)

#define LOOP\
    for(;;)

#define HANDLER_EPOLL_EVENT(count, ev, data, __events) \
    ev = __events[0];\
    data = ev.data.ptr;\
    for (int i = 0; i < count; ev = __events[++i], data = ev.data.ptr)

/**
 * 对于服务端这是访问端
 * 对于连接端这是被访问端
*/
#define CFRP_RFD(frp) (frp->sock + 1)->sfd 

/**
 * 对于服务端这是映射连接端
 * 对于连接端这是连接到服务端
*/
#define CFRP_LFD(frp) frp->sock->sfd


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
    map_init(&frp->mappers, 20);
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
    memcpy(frp->peers, peers, sizeof(c_peer) * 2);
    if( make_tcp(peers, frp->sock)             == CFRP_ERR || 
        make_tcp((peers + 1), frp->sock + 1)   == CFRP_ERR ){
        perror("cfrp make error");
        exit(1);
    } 
    if( setnoblocking(CFRP_LFD(frp))          == CFRP_ERR ||
        setnoblocking(CFRP_RFD(frp))          == CFRP_ERR ){
        perror("cfrp set noblocking error");
        exit(1);
    }
    if ( (frp->efd = make_epoll())            ==  CFRP_ERR){
        perror("cfrp epoll error");
        exit(0);
    }
    if(EPOLL_ADD(frp, CFRP_LFD(frp), NULL)   < 0 ||
       EPOLL_ADD(frp, CFRP_RFD(frp), NULL)   < 0){
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
    memcpy(frp->peers, peers, sizeof(c_peer) * 2);
    if( make_connect(peers, frp->sock) == CFRP_ERR ||
        setnoblocking(CFRP_LFD(frp))   == CFRP_ERR ||
        (frp->efd = make_epoll())      == CFRP_ERR){
            perror("cfrp make error");
            exit(1);
    }
    if(EPOLL_ADD(frp, CFRP_LFD(frp), NULL) < 0){
        perror("epoll error");
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
    int rfd = CFRP_RFD(frp), lfd = CFRP_LFD(frp), mfd = -1, cfd = -1;
    struct epoll_event events[5], ev;
    cfrp_epoll_data* data;
    c_sock sock;
    char *uuid = (void*)0;
    LOG_INFO("server started !\nMapping: [%s:%d]:%d->[%s:%d]:%d", 
                frp->sock[1].peer.addr,
                frp->sock[1].peer.port,
                frp->sock[1].sfd,
                frp->sock->peer.addr,
                frp->sock->peer.port,
                frp->sock->sfd);
    LISTEN{
        __DEF_EPOLL_WAIT__(c, frp->efd, events, 5, -1);
        mfd = frp->sock[2].sfd;
        bzero(&sock, sizeof(c_sock));
        HANDLER_EPOLL_EVENT(c, ev, data, events){
            cfd  = data->sfd;
            if(cfd == rfd || cfd == lfd){
                if( cfrp_accept(cfd, &sock) == CFRP_ERR || 
                    setnoblocking(sock.sfd) == CFRP_ERR){
                    perror("cfrp accept error");
                    continue;
                }
                if(cfd == rfd && mfd <= 0 || cfd == lfd && mfd > 0){
                    /**
                     * 被映射端还没有连接或者被映射端已经被占用, 拒绝连接
                    */
                   cfrp_close(frp, sock.sfd);
                   continue;
                }else if(cfd == lfd){
                    /**
                     * 被映射端连接
                    */
                    LOG_INFO("client connect: %d", sock.sfd);
                    memcpy(frp->sock + 2, &sock, sizeof(c_sock));
                }else{
                    /**
                     * 注册监听
                    */
                    LOG_INFO("register mapping");
                    uuid = cfrp_register(frp, &sock);
                }
                // 注册监听
                EPOLL_ADD(frp, sock.sfd, uuid);
            }else{
                int s = CFRP_CONN;
                if(mfd == cfd){
                    LOG_INFO("recv forward");
                    s = cfrp_recv_forward(frp);
                }else{
                    LOG_INFO("send forward");
                    s = cfrp_send_forward(frp, data->ptr);
                }
                if(s == CFRP_DISCONNECT || s == CFRP_STOP){
                    cfrp_close(frp, cfd);
                    continue;
                }
            }
            EPOLL_MOD(frp, cfd, &ev);
        }
    }
}

/**
 * 启动客户端
*/
extern int run_client(cfrp* frp){
    struct epoll_event events[10], ev;
    cfrp_epoll_data *data;
    int cfd = -1, lfd = -1;
    LISTEN{
        __DEF_EPOLL_WAIT__(c, frp->efd, events, 5, -1);
        lfd = CFRP_LFD(frp);
        HANDLER_EPOLL_EVENT(c, ev, data, events){
            cfd = data->sfd;
            int s;
            if(cfd == lfd){
                LOG_INFO("recv server message");
                /**
                 * 服务端传来了数据需要将数据解析然后转发的目标地址
                */
                s = cfrp_recv_forward(frp);
            }else{
                s = cfrp_send_forward(frp, data->ptr);
            }
            if( s == CFRP_STOP || s == CFRP_DISCONNECT){
                cfrp_stop(frp);
                break;
            }else if(s == CFRP_DISCONNECT){
                cfrp_close(frp, cfd);
                free(map_remove(&frp->mappers, data->ptr));
            }else{
                EPOLL_MOD(frp, cfd, &ev);
            }
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
    }else if (pid == 0){
        if(frp->type == SERVER){
            run_server(frp);
        }else{
            run_client(frp);
            exit(0);
        }
    }
    return 1;
}


/**
 * 停止服务
*/
extern int  cfrp_stop(cfrp* frp){
    if(frp->type = CLIENT){
        if(frp->sock)
            cfrp_close(frp, frp->sock->sfd);
    }
    frp->efd = -1;
    LOG_INFO("cfrp stop, code: %d, message: %s", errno, strerror(errno));
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
    sock->peer.addr = peer->addr;
    sock->peer.port = peer->port;
    return CFER_SUCC;
}

/**
 * 创建一个tcp连接端
*/
extern int make_connect(c_peer* peer, c_sock *sock){
    if(! peer) return CFRP_ERR;
    LOG_INFO("connect: [%s:%d]", peer->addr, peer->port);
    int fd = -1;
    struct sockaddr_in addr = SOCK_ADDR_IN(peer);
    if( (fd = socket(AF_INET, SOCK_STREAM, 0))                < 0 ||
        connect(fd, SOCKADDR(&addr), sizeof(struct sockaddr)) < 0 ){
        return CFRP_ERR;
    }
    sock->sfd = fd;
    sock->peer.addr = peer->addr;
    sock->peer.port = peer->port;
    return fd;
}

extern cfrp_head* make_head(){
    cfrp_head* head  = (cfrp_head*)malloc(sizeof(cfrp_head));
    bzero(head, sizeof(cfrp_head));
    return head;
}

extern int cfrp_listen(cfrp* frp, int sfd, int op, int events, void* __data){
    cfrp_epoll_data *data  = malloc(sizeof(cfrp_epoll_data));
    data->sfd = sfd;
    data->ptr = __data;
    __DEF_EPOLL_EVENT__(ev, events, data);
    if(epoll_ctl(frp->efd, op, sfd, &ev) < 0 )
        return CFRP_ERR;
    return CFER_SUCC;
}

extern int cfrp_accept(int fd, c_sock* sock){
    bzero(sock,  sizeof(c_sock));
    int afd = -1;
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    if((afd = accept(fd, SOCKADDR(&addr), &len)) < 0)
        return CFRP_ERR;
    SOCK_PEER(&addr, sock->peer); 
    sock->sfd = afd;
    return  CFER_SUCC;
}

extern int cfrp_close(cfrp* frp, int fd){
    LOG_DEBUG("close connect: fd: %d", fd);
    if(frp->sock[2].sfd == fd || frp->sock->sfd == fd){
        cfrp_clear_mappers(frp);
        if(frp->type == SERVER)
            bzero(frp->sock + 2, sizeof(c_sock));
        else if (frp->type == CLIENT)
            bzero(frp->sock, sizeof(c_sock));
    }
    EPOLL_DEL(frp, fd, NULL);
    if(shutdown(fd, SHUT_RDWR) < 0 || close(fd))
        return CFRP_ERR;
    return CFER_SUCC;
}

/**
 * 转发接收到的信息
*/
extern int cfrp_recv_forward(cfrp* frp){   
    cfrp_head head; 
    char buff[CFRP_BUFF_SIZE], *code;
    int sfd, tfd, l, hs, cs, st, r;
    sfd = frp->type == SERVER ? frp->sock[2].sfd : frp->sock->sfd;
    hs = sizeof(cfrp_head); cs = hs, st = 0;
    LOOP{
        bzero(buff, CFRP_BUFF_SIZE);
        l = recv(sfd, buff, cs, 0);
        LOG_DEBUG("recv size: expected: %d, current: %d, p: %d", cs, l, st);
        if(l == -1 && errno == EAGAIN){
            r = CFRP_WAIT;
            break;
        }else if(l == 0 && errno == EAGAIN){
            r = CFRP_DISCONNECT;
            break;
        }
        if(l != cs){
            r = CFRP_DISCONNECT;
            break;
        }
        if(st == 0){
            memcpy(&head, buff, cs);
            if(GMASK1(head.mask) != 0){
                LOG_DEBUG("head err");
                r = CFRP_DISCONNECT;
                break;
            }
            cs = GMASK2(head.mask);
            if(cs == 0) {
                cs = GMASK3(head.mask);
                st++;
            }
            st++;
        }else if(st == 1){
            LOG_DEBUG("code: %s", buff);
            code = malloc(sizeof(char) * l);
            memcpy(code, buff, l);
            cs = GMASK3(head.mask);
            st++;
        }else{
            c_sock* sock = map_get(&frp->mappers, code);
            if(frp->type == CLIENT && !sock){
                sock = malloc(sizeof(c_sock));
                if( make_connect(frp->peers + 1, sock) != CFRP_ERR &&
                    setnoblocking(sock->sfd) != CFRP_ERR){
                    EPOLL_ADD(frp, sock->sfd, code);
                    map_put(&frp->mappers, code, sock);
                }else{
                    free(sock);
                    sock = (void*)0;
                }
            }
            LOG_DEBUG("forward: [%s:%d], fd: %d", sock->peer.addr, sock->peer.port, sock->sfd);
            if(!sock || send(sock->sfd, buff, l, 0) < 0){
                r = CFRP_DISMAPPER;
            }
            LOG_DEBUG("forward success");
            st = 0;
            cs = hs;
            r = CFRP_CONN;
            bzero(&head, hs);
        }
    }
    return r;
}


extern int cfrp_send_forward(cfrp* frp, char* uuid){
    cfrp_head head;
    c_sock* sock = map_get(&frp->mappers, uuid);
    char buff[CFRP_BUFF_SIZE];
    int hs, bs, sfd, tfd, l, r, ul;
    hs = sizeof(cfrp_head); sfd = sock->sfd; bs = sizeof(buff); ul = strlen(uuid); 
    tfd = frp->type == SERVER ? frp->sock[2].sfd : frp->sock->sfd;
    LOOP{
        bzero(&head, hs);
        bzero(buff, bs);
        l = recv(sfd, buff, bs, 0);
        if( l == -1 && errno == EAGAIN){
            r = CFRP_WAIT;
            break;
        }else if(l == 0 && errno == EAGAIN){
            r = CFRP_DISCONNECT;
            break;
        }
        head.mask = cfrp_mask(head.mask, 0, MASK_1);
        head.mask = cfrp_mask(head.mask, ul, MASK_2);
        head.mask = cfrp_mask(head.mask, l, MASK_3);
        char sbuff[l + hs + ul];
        memcpy(sbuff, &head, hs);
        memcpy(sbuff + hs, uuid, ul);
        memcpy(sbuff + hs + ul, buff, l);
        LOG_DEBUG("forward size: head: %d, body: %d, sum: %d", hs, l, hs + l + ul);
        if(send(tfd, sbuff, l + hs + ul, 0) < 0){
            r = CFRP_DISCONNECT;
            break;
        }
        
    }
    LOG_DEBUG("forward success");
    return r;
}


extern char* cfrp_uuid(unsigned int max){
    max = max < 10 ? 10: max;
    char buff[max], chr;
    memset(buff, '\0', sizeof(buff));
    int i = 0;
    for(; i < max; i++){
        if(rand() % 2){
            chr = __BASE_LATTER__[rand() % 26];
            chr = rand() %  2 ? chr - 32 : chr;
            buff[i] = chr;
        }else{
            buff[i] = chr = rand() % 9 + '0';
        }
    }
    char* mbuff = malloc(sizeof(chr) * sizeof(buff));
    memset(mbuff, '\0', sizeof(mbuff));
    memcpy(mbuff, buff, i);
    return mbuff;
}

extern char* cfrp_register(cfrp* frp, c_sock* sock){
    char* uuid = cfrp_uuid(18);
    while (map_get(&frp->mappers, uuid))
        uuid = cfrp_uuid(18);
    c_sock* msock = malloc(sizeof(c_sock));
    memcpy(msock, sock, sizeof(c_sock));
    map_put(&frp->mappers, uuid, msock);
    return uuid;
}

extern char* cfrp_clear_mappers(cfrp* frp){
    clist list;
    bzero(&list, sizeof(clist));
    map_keys(&frp->mappers, &list);
    c_sock *sock;
    for(int i = 0; i < list.size; i++){
        sock = map_remove(&frp->mappers, list_get(&list, i));
        if(! sock)continue;
        cfrp_close(frp, sock->sfd);
        free(sock);
    }
}
