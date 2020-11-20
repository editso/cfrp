#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <time.h>

#include "cfrp.h"
#include "logger.h"
#include "cbuff.h"
#include "clib.h"

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
extern int cfrp_send_forward(cfrp* frp, cfrp_token* tok);

/**
 * 注册监听
*/
extern cfrp_token* cfrp_register(cfrp* frp, c_sock* sock);
/**
 * 取消注册
*/
extern char* cfrp_unregister(cfrp* frp, c_sock* sock);

extern void cfrp_clear_mappers(cfrp* frp);




/**
 * 主机地址转换为网络地址
*/
#define SOCK_ADDR_IN(peer) \
{\
        .sin_family = AF_INET,\
        .sin_port = htons(peer->port),\
        .sin_addr = {.s_addr = inet_addr(peer->addr)}\
}


/**
 * 主机地址信息
*/
#define SOCK_PEER(__addr, peer)\
    peer.port = ntohs((__addr)->sin_port);\
    peer.addr = inet_ntoa((__addr)->sin_addr);\


/**
 * sockaddr_in to sockaddr
 * 
*/
#define SOCKADDR(addr) (struct sockaddr*)addr

/**
 * epoll_event data
*/
typedef struct{
    int sfd;
    void *ptr;
}cfrp_epoll_data;


/**
 * epoll 水平触发可读事件
*/
#define CFRP_EPOLL_EVENT EPOLLET|EPOLLIN

#define __DEF_EPOLL_EVENT__(__vname, __events, __data) \
    struct epoll_event __vname;\
    __vname.events = __events;\
    __vname.data.ptr = __data;
    
#define __DEF_EPOLL_WAIT__(__vname, efd, __events,__max, __timeout) \
    if(efd == -1) break;\
    int __vname = epoll_wait(efd, __events,  __max, __timeout);

/**
 * 向epoll中注册一个事件
*/
#define EPOLL_ADD(__frp, __sfd, __data) cfrp_listen(__frp, __sfd, EPOLL_CTL_ADD, CFRP_EPOLL_EVENT, __data)
/**
 * 重新注册
*/
#define EPOLL_MOD(__frp, __sfd, __ev) epoll_ctl(__frp->efd, EPOLL_CTL_MOD, __sfd, __ev)
/**
 * 删除事件
*/
#define EPOLL_DEL(__frp, __sfd, __ev) epoll_ctl(__frp->efd, EPOLL_CTL_DEL, __sfd, __ev)

/**
 * 监听
*/
#define LISTEN\
    for(;;)

/**
 * 循环
*/
#define LOOP\
    for(;;)

/**
 * 处理epoll 事件
*/
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
 * 设置cfrp转发头mask
*/
#define HEAD_MASK(__head, v, p) (__head)->mask = cfrp_mask((__head)->mask, v, p) 


/**
 * 设置非阻塞IO
*/
extern int setnoblocking(int fd){
    int flag;
    if((flag = fcntl(fd, F_GETFL, 0)) < 0){
        return CFRP_ERR;
    }
    if(fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0){
        return CFRP_ERR;
    }
    return CFRP_SUCC;
}


/**
 * 打包
 * 
*/
extern char* cfrp_pack(cfrp_head *head){
    int size = sizeof(cfrp_head);
    char* _pack = (char *)malloc(sizeof(char) * size);
    cbzero(_pack, sizeof(char) * size);
    memcpy(_pack, head, size);
    return _pack;
}


/**
 * 解包
*/
extern cfrp_head* cfrp_unpack(char* data){
    int size = sizeof(cfrp_head);
    cfrp_head* _pack = (cfrp_head *)malloc(size);
    cbzero(_pack, size);
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
    cbzero(frp, sizeof(cfrp));
    map_init(&frp->mappers, 20);
    cbzero(&frp->cache, sizeof(cqueue));
    frp->cache.capacity = 5;
    frp->cache._elems = calloc(frp->cache.capacity, sizeof(void *));
    frp->state.op = 0;
    return frp;
}


extern int make_epoll(){
    int efd = epoll_create(EPOLL_SIZE);
    if(efd < 0) return CFRP_ERR;
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
    cfrp_token* tok;
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
        cbzero(&sock, sizeof(c_sock));
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
                     * 被映射端连接,
                    */
                    LOG_INFO("client connect: %d", sock.sfd);
                    memcpy(frp->sock + 2, &sock, sizeof(c_sock));
                }else{
                    /**
                     * 注册监听并生产一个唯一标识
                    */
                    LOG_INFO("register mapping");
                    tok = cfrp_register(frp, &sock);
                }
                // 注册监听
                EPOLL_ADD(frp, sock.sfd, tok);
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
                    tok = data->ptr;
                    free(map_remove(&frp->mappers, tok->token));
                    free(tok);
                    continue;
                }
            }
            EPOLL_MOD(frp, cfd, &ev);
        }
    }
    return 1;
}


/**
 * 启动客户端
*/
extern int run_client(cfrp* frp){
    struct epoll_event events[10], ev;
    cfrp_epoll_data *data;
    cfrp_token* tok;
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
                s = cfrp_recv_forward(frp) != CFRP_SUCC ? CFRP_STOP : CFRP_SUCC;
            }else{
                /**
                 * 映射端传来了数据, 打包发送给服务端
                */
                s = cfrp_send_forward(frp, data->ptr);
            }
            if( s == CFRP_STOP ){
                cfrp_stop(frp);
            }else if(s == CFRP_DISCONNECT){
                cfrp_close(frp, cfd);
                tok = data->ptr;
                free(map_remove(&frp->mappers, tok->token));
                free(tok);
            }else{
                EPOLL_MOD(frp, cfd, &ev);
            }
        }
    }
    return 1;
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
        }
        exit(0);
    }
    return 1;
}


/**
 * 停止服务
*/
extern int  cfrp_stop(cfrp* frp){
    if(frp->type == CLIENT){
        if(frp->sock)
            cfrp_close(frp, frp->sock->sfd);
    }
    frp->efd = -1;
    LOG_INFO("cfrp stop, code: %d, message: %s", errno, strerror(errno));
    return 1;
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
    return CFRP_SUCC;
}

/**
 * 创建一个tcp连接端
*/
extern int make_connect(c_peer* peer, c_sock *sock){
    if(! peer) return CFRP_ERR;
    int fd = -1;
    struct sockaddr_in addr = SOCK_ADDR_IN(peer);
    if( (fd = socket(AF_INET, SOCK_STREAM, 0))                < 0 ||
        connect(fd, SOCKADDR(&addr), sizeof(struct sockaddr)) < 0 ){
        return CFRP_ERR;
    }
    sock->sfd = fd;
    sock->peer.addr = peer->addr;
    sock->peer.port = peer->port;
    LOG_INFO("connected: [%s:%d], fd: %d", peer->addr, peer->port, fd);
    return fd;
}


extern cfrp_head* make_head(){
    cfrp_head* head  = (cfrp_head*)malloc(sizeof(cfrp_head));
    cbzero(head, sizeof(cfrp_head));
    return head;
}

extern int cfrp_listen(cfrp* frp, int sfd, int op, int events, void* _data){
    cfrp_epoll_data *data  = malloc(sizeof(cfrp_epoll_data));
    data->sfd = sfd;
    data->ptr = _data;
    __DEF_EPOLL_EVENT__(ev, events, data);
    if(epoll_ctl(frp->efd, op, sfd, &ev) < 0 )
        return CFRP_ERR;
    return CFRP_SUCC;
}

extern int cfrp_accept(int fd, c_sock* sock){
    cbzero(sock,  sizeof(c_sock));
    int afd = -1;
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    if((afd = accept(fd, SOCKADDR(&addr), &len)) < 0)
        return CFRP_ERR;
    SOCK_PEER(&addr, sock->peer); 
    sock->sfd = afd;
    return  CFRP_SUCC;
}

extern int cfrp_close(cfrp* frp, int fd){
    LOG_DEBUG("close connect: fd: %d", fd);
    if(frp->sock[2].sfd == fd || frp->sock->sfd == fd){
        cfrp_clear_mappers(frp);
        if(frp->type == SERVER)
            cbzero(frp->sock + 2, sizeof(c_sock));
        else if (frp->type == CLIENT)
            cbzero(frp->sock, sizeof(c_sock));
    }
    EPOLL_DEL(frp, fd, NULL);
    if(shutdown(fd, SHUT_RDWR) < 0 || close(fd))
        return CFRP_ERR;
    return CFRP_SUCC;
}


extern int cfrp_recv(cfrp* frp, int fd, char* buff, int size){
    if(size <= 0)
        return CFRP_ERR;   
    char mbuff[size];
    cbuff* mcache = queue_pop(&frp->cache);
    if(! mcache)
        mcache = make_buff(size);
    int l = recv(fd, mbuff, size - mcache->length, 0);
    if(l == -1 && errno == EAGAIN){
        return CFRP_WAIT;
    }else if(l == 0 && errno == EAGAIN){
        return CFRP_ERR;
    }
    buff_appends(mcache, mbuff, l);
    LOG_INFO("total: %d, current: %d, sub: %d", size, l, size - mcache->length);
    if(mcache->length != size){
        queue_push(&frp->cache, mcache);
        return CFRP_WAIT;
    }
    buff_sub(mcache, buff, 0, size);
    // 回收内存
    buff_recycle(mcache);
    free(mcache);
    return size;
}

/**
 * 转发接收到的信息
*/
extern int cfrp_recv_forward(cfrp* frp){   
    cfrp_state* state = &frp->state;
    cfrp_head *head = &frp->state.head; 
    cfrp_token *tok = &state->tok;
    int sfd, l, hl, cl, *st, r, sl;
    sfd = frp->type == SERVER ? frp->sock[2].sfd : frp->sock->sfd;
    char buff[CFRP_BUFF_SIZE];
    hl = sizeof(cfrp_head), cl = hl, st = &frp->state.op;
    LOOP{
        r = CFRP_SUCC;
        cbzero(buff, CFRP_BUFF_SIZE);
        if(*st == 1){
            cl = GMASK2(head->mask);
        }else if(*st == 2){
            cl = GMASK3(head->mask);
        }
        if((l = cfrp_recv(frp, sfd, buff, cl)) == CFRP_ERR){
            r = CFRP_DISCONNECT;
            break;
        }else if (l == CFRP_WAIT){
            break;
        }
        LOG_DEBUG("recv size: expected: %d, current: %d, p: %d", cl, l, *st);
        if(*st == 0){
            memcpy(head, buff, cl);
            if(GMASK1(head->mask) == 0x00){
                LOG_DEBUG("header error");
                r = CFRP_DISCONNECT;
                break;
            }
           (*st)++;  
        }else if(*st == 1){
            tok->len = cl;
            tok->token = calloc(cl, sizeof(char));
            memcpy(tok->token, buff, cl);
            LOG_DEBUG("token: %s, length: %d", tok->token, cl);
            (*st)++;
        }else{
            c_sock* sock = map_get(&frp->mappers, tok->token);
            if(frp->type == CLIENT && !sock){
                sock = calloc(1, sizeof(c_sock));
                if( make_connect(frp->peers + 1, sock) != CFRP_ERR &&
                    setnoblocking(sock->sfd) != CFRP_ERR){
                    cfrp_token* stok = make_cfrp_token(tok->len, tok->token);
                    EPOLL_ADD(frp, sock->sfd, stok);
                    map_put(&frp->mappers, stok->token, sock);
                }else{
                    free(sock);
                    sock = (void*)0;
                }
            }
            LOG_DEBUG("forward: [%s:%d], fd: %d", sock->peer.addr, sock->peer.port, sock->sfd);
            if(!sock || (sl = send(sock->sfd, buff, l, 0)) < 0){
                r = CFRP_DISMAPPER;
                break;
            }
            LOG_DEBUG("send: total: %d, current: %d", l, sl);
            (*st) = 0;
            cl = hl;
            cbzero(head, hl);
            free(tok->token);
            cbzero(tok, sizeof(cfrp_token));
        }
    }
    return r;
}


extern int cfrp_send_forward(cfrp* frp, cfrp_token* tok){
    cfrp_head head;
    c_sock* sock = map_get(&frp->mappers, tok->token);
    char buff[CFRP_BUFF_SIZE];
    int hs, bs, sfd, tfd, l, r, sl, total;
    hs = sizeof(cfrp_head); sfd = sock->sfd; bs = sizeof(buff); 
    tfd = frp->type == SERVER ? frp->sock[2].sfd : frp->sock->sfd;
    LOOP{
        r = CFRP_SUCC;
        cbzero(&head, hs);
        cbzero(buff, bs);
        l = recv(sfd, buff, bs, 0);
        if( l == -1 && errno == EAGAIN){
            r = CFRP_WAIT;
            break;
        }else if(l == 0 && errno == EAGAIN){
            r = CFRP_DISCONNECT;
            break;
        }
        HEAD_MASK(&head, 0x01, MASK_1);
        HEAD_MASK(&head, tok->len, MASK_2);
        HEAD_MASK(&head, l, MASK_3);
        total = l + hs + tok->len;
        char sbuff[total];
        memcpy(sbuff, &head, hs);
        memcpy(sbuff + hs, tok->token, tok->len);
        memcpy(sbuff + hs + tok->len, buff, l);
        LOG_DEBUG("forward size: head: %d, token: %d, body: %d, total: %d", hs, tok->len, l, total);
        if((sl = send(tfd, sbuff, total, 0)) < 0){
            r = CFRP_DISCONNECT;
            break;
        }
        LOG_DEBUG("send: total: %d, current: %d", total, sl);
    }
    if(r == CFRP_SUCC)
        LOG_INFO("forward success");
    else
        LOG_ERROR("forward error, code: %d, message: %s", errno, strerror(errno));
    return r;
}

extern int cfrp_gentok(char* dest, unsigned int max){
    max = max < 10 ? 10: max;
    char buff[max], chr;
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
    cbzero(dest, max);
    memcpy(dest, buff, i);
    return CFRP_SUCC;
}

extern cfrp_token* cfrp_register(cfrp* frp, c_sock* sock){
    char token[18];
    do{
        cfrp_gentok(token, 18);
    } while (map_get(&frp->mappers, token));
    cfrp_token* tok = make_cfrp_token(18u, token);
    c_sock* msock = malloc(sizeof(c_sock));
    memcpy(msock, sock, sizeof(c_sock));
    map_put(&frp->mappers, tok->token, msock);
    return tok;
}


extern void cfrp_clear_mappers(cfrp* frp){
    clist list;
    cbzero(&list, sizeof(clist));
    map_keys(&frp->mappers, &list);
    c_sock *sock;
    for(int i = 0; i < list.size; i++){
        sock = map_remove(&frp->mappers, list_get(&list, i));
        if(!sock) continue;
        cfrp_close(frp, sock->sfd);
        free(sock);
    }
}


extern cfrp_token* make_cfrp_token(unsigned int len, char* token){
    cfrp_token* tok = calloc(1, sizeof(cfrp_token));
    tok->len = len;
    tok->token = calloc(len, sizeof(char));
    memcpy(tok->token, token, len);
    return tok;
}