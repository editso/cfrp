#include "cfrp.h"
#include "logging.h"

#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <map>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>


using namespace std;

#define MAX_CACHE 2048 * 1024

namespace rpr
{
    


    typedef struct{
        int fd;
        void *ptr;
    }Data;


    /**
     * return 成功 1 否则 0
    */
    int make_tcp(struct peer peer, r_sock *sock)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(peer.port),
            .sin_addr = {.s_addr = inet_addr(peer.addr)}};
        int opt = 1;
        if (fd < 0 ||
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0 ||
            bind(fd, (struct sockaddr *)&addr, sizeof(sockaddr_in)) < 0 ||
            listen(fd, 10) < 0){
            logger->error("make_tcp");
            return 0;
        }else{
            sock->peer = peer;
            sock->sockfd = fd;
        }
        printf("listen: %s:%d\n", peer.addr, peer.port);
        return 1;
    }

    int make_connect(struct peer peer, r_sock *sock){
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(peer.port),
            .sin_addr = {
                .s_addr = inet_addr(peer.addr)
            }
        };
        if(connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0){
            return 0;
        }
        sock->sockfd = fd;
        sock->peer = peer;
        return 1;
    }

    int setnoblocking(int fd){
        int flags;
        if((flags = fcntl(fd, F_GETFL, 0)) < 0){
            return -1;
        }
        flags |= O_NONBLOCK;
        if(fcntl(fd, F_SETFL,flags)  < 0){
            return -1;
        }
        return true;
    }

    int handle_agent(int fd, int efd, map<int, r_sock> &mappers){
        Message msg;
        char buff[MAX_CACHE];
        int s1 = -1, s2 = -1, b_size = sizeof(buff);
        r_sock sock;
        while(1){
            s1 = recv(fd, &msg, sizeof(msg), 0);
            printf("agent len: %d", msg.len);
            s2 = recv(fd, buff, msg.len, 0);
            if(s1 == -1 || s2 == -1  && errno == EAGAIN){
                break;
            }
            if(s1 == -1 || s2 == -1  && errno == EAGAIN){
                logger->info("epoll disconnect");
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                return  -1;
            }
            try{
                sock = mappers.at(msg.id);    
            }catch(const std::exception& e){
                logger->error("no exists, id: " + to_string(msg.id));
                return -1;
            }
           
            if(msg.len < 0  || send(sock.sockfd, buff, s2, 0) < 0){
                logger->error("send error");
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
            }
        }
        return 1;
    }

    int handle_message(int fd, int tfd, int efd, unsigned int id){
        Message msg;
        char buff[MAX_CACHE];
        int s;
        char mbuff[sizeof(msg) + sizeof(buff)];
        while(1){
            bzero(buff, sizeof(buff));
            bzero(&msg, sizeof(msg));
            s = recv(fd, buff, sizeof(buff), 0);
            if(s == -1 && errno == EAGAIN){
                logger->info("send finish");
                break;
            }
            if(s == 0 && errno == EAGAIN){
                epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                return -1;
            }
            msg.id = id;
            msg.len = s;
            printf("len: %d, fd: %d, errno: %d\n", s, fd, errno);
            memcpy(mbuff, &msg, sizeof(msg));
            memcpy(mbuff + sizeof(msg), buff, s);
            if(send(tfd, mbuff, sizeof(Message) + s, 0) < 0){
                perror("send error");
                epoll_ctl(efd, EPOLL_CTL_DEL, tfd, NULL);
                return -1;
            }
        }
        return 1;
    }

    int epoll_add(int efd, int sockfd, int events, void* data){
        struct epoll_event *_event = (struct epoll_event*)malloc(sizeof(struct epoll_event)); 
        Data *_data = (Data*)malloc(sizeof(Data));
        _event->events = events;
        // _event->data.ptr = data;
        // _event->data.fd = sockfd;
        _data->fd = sockfd;
        _data->ptr = data;
        _event->data.ptr = _data;
        return epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, _event);
    }

    class RServer : public Mapper
    {
        private:
            r_sock listen_mapper;
            r_sock client;
            r_sock mapper = {.sockfd = -1};
            map<int, r_sock> mappers;
            int efd = -1;
        
        public:
            RServer(struct peer mapper[]) : Mapper(mapper)
            {
                
                if( !make_tcp(this->get(0), &this->listen_mapper) || 
                    !make_tcp(this->get(1), &this->client)  ){
                    exit(1);
                }
                if(
                    !setnoblocking(this->listen_mapper.sockfd) ||
                    !setnoblocking(this->client.sockfd)
                ){
                    perror("set no blocking error");
                    exit(1);
                }
                if((this->efd = epoll_create(10)) < 0){
                    perror("epoll error");
                    exit(1);
                }
            }

            int check_mapper(){
                return  this->mapper.sockfd != -1;
            }

            void start(){
                logger->debug("start server");
                if( epoll_add(this->efd, this->client.sockfd, EPOLLIN|EPOLLET, NULL) < 0 ||
                    epoll_add(this->efd, this->listen_mapper.sockfd, EPOLLIN|EPOLLET, NULL) < 0  ){
                        perror("epoll");
                        exit(1);
                }
                struct epoll_event events[5], ev;
                sockaddr_in in;
                socklen_t len = sizeof(sockaddr_in);
                r_sock *sock;
                Data *data;
                for(;;)
                {
                    bzero(&ev, sizeof(ev));
                    bzero(events, sizeof(events));
                    int c = epoll_wait(this->efd, events, 10, -1), fd = -1;
                    
                    for (size_t i = 0; i < c; i++)
                    {
                        ev = events[i];
                        data = (Data*)ev.data.ptr;
                        // fd = ev.data.fd;
                        fd = data->fd;
                        if(this->listen_mapper.sockfd == fd || this->client.sockfd == fd){
                            logger->debug("accept client or mapper");
                            sock = (r_sock*)malloc(sizeof(r_sock));
                            bzero(&in, sizeof(in));
                            bzero(sock, sizeof(sock));
                            int _fd = accept(fd , (struct sockaddr*)&in, &len);
                            int _stat = this->check_mapper();
                            if( !_stat && this->client.sockfd == fd || _stat && this->listen_mapper.sockfd == fd){
                                shutdown(_fd, SHUT_RDWR);
                                continue;
                            }
                            if(setnoblocking(_fd) < 0){
                                perror("set no blocking error");
                                continue;
                            }
                            sock->peer.port = ntohs(in.sin_port);
                            sock->peer.addr = inet_ntoa(in.sin_addr);
                            sock->sockfd = _fd;
                            if(this->client.sockfd  == fd){
                                logger->debug("add client");
                                mappers.insert(pair<int, r_sock>(sock->sockfd, *sock));
                            }else{
                                this->mapper.peer = {sock->peer.port, sock->peer.addr};
                                this->mapper.sockfd =  _fd;
                            }
                            epoll_add(this->efd, _fd, EPOLLET|EPOLLIN, NULL);
                            epoll_ctl(this->efd, EPOLL_CTL_MOD, fd, &ev);
                        }else if(ev.events & EPOLLIN){
                            if(this->mapper.sockfd == fd){
                                // receive mapper message
                                logger->debug("receive mapper message");
                                if(handle_agent(fd,this->efd, this->mappers)){
                                    epoll_ctl(this->efd, EPOLL_CTL_MOD, fd, &ev);
                                }
                            }else{
                                // receive client message
                                logger->debug("receive client message");
                                if(handle_message(fd, this->mapper.sockfd, this->efd, fd) < 0){
                                    shutdown(fd, SHUT_RDWR);
                                }else{
                                    epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ev);
                                }
                            }
                        }
                    }
                }
            
            }

            void stop(){
                    
            }
    };

    class RClient : public Mapper
    {   
        struct peer client;
        struct peer mapper;
        map<int, r_sock> mappers;
        r_sock r_client = {.sockfd = -1};
        int efd = -1;

        public:
            RClient(struct peer mapper[]) : Mapper(mapper)
            {
                this->client = this->get(0);
                this->mapper = this->get(1);
                this->efd = epoll_create(10);
            }

            void start()
            {

                if( !make_connect(this->mapper, &this->r_client) || 
                    setnoblocking(this->r_client.sockfd) < 0){
                    perror("connect server err");
                    exit(1);
                }
                epoll_add(this->efd, this->r_client.sockfd, EPOLLET|EPOLLIN, NULL);
                struct epoll_event events[5], ev;
                int fd, c = 0, l = 0;
                char buff[MAX_CACHE]; 
                Message *msg;
                r_sock *sock;
                Data* data;
                for (;;)
                {
                    bzero(events, sizeof(events));
                    bzero(&ev, sizeof(ev));
                    c = epoll_wait(this->efd, events, 5, -1);
                    for (int i = 0; i < c; i++)
                    {
                        ev = events[i];
                        data = (Data*)ev.data.ptr;
                        // fd = ev.data.fd;
                        fd = data->fd;
                        if(fd == this->r_client.sockfd){
                            // receive mapper message
                            bzero(buff, sizeof(buff));
                            while(1){
                                msg = (Message*)malloc(sizeof(Message));
                                bzero(msg, sizeof(Message));
                                l = recv(fd, msg, sizeof(Message), 0);
                                if(l == -1 && errno == EAGAIN){
                                    logger->info("recv finish");
                                    break;
                                }else if (l == 0 && errno == EAGAIN)
                                {
                                    epoll_ctl(this->efd, EPOLL_CTL_DEL, fd, &ev);
                                    logger->error("client disconnect");
                                    break;
                                }

                                try{
                                    sock = &this->mappers.at(msg->id);
                                }catch(const std::exception &e){
                                    sock = (r_sock*)malloc(sizeof(r_sock));
                                    if( !make_connect(this->client, sock) || 
                                        setnoblocking(sock->sockfd) < 0){
                                        perror("connect proxy error");
                                        break;
                                    }
                                    this->mappers.insert(pair<int, r_sock>(msg->id, *sock));
                                    Message emsg = {msg->id, msg->len};
                                    if(epoll_add(this->efd, sock->sockfd, EPOLLIN|EPOLLET, &emsg) < 0 ){
                                        perror("epoll error");
                                    }
                                }
                                l = recv(fd, buff, msg->len, 0);
                                free(msg);
                                if(l == -1 && errno == EAGAIN){
                                    break;
                                }else if (l == 0 && errno == EAGAIN)
                                {
                                    epoll_ctl(this->efd, EPOLL_CTL_DEL, fd, &ev);
                                    logger->error("client disconnect");
                                    break;
                                }
                                if(send(sock->sockfd, buff, l, 0) < 0){
                                    perror("send to proxy error");
                                    epoll_ctl(this->efd, EPOLL_CTL_DEL, sock->sockfd, NULL);
                                }
                            }
                            epoll_ctl(this->efd, EPOLL_CTL_MOD, fd, &ev);
                        }else{
                            // receive proxy message
                            logger->info("receive proxy message");
                            msg = (Message*)data->ptr;
                            if( handle_message(fd, this->r_client.sockfd, this->efd, msg->id) < 0){
                                shutdown(fd, SHUT_RDWR);
                                free(msg);
                            }else{
                                epoll_ctl(this->efd, EPOLL_CTL_MOD, fd, &ev);
                            }
                        }
                    }
                    
                }
                
            }

            void stop()
            {
                
            }
    };

    extern Mapper *make_server(struct peer mapper[])
    {
        return new RServer(mapper);
    }

    extern Mapper *make_client(struct peer mapper[])
    {
        return new RClient(mapper);
    }
} // namespace rpr
