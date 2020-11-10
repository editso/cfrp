#include <iostream>

#ifndef __PROXY_H__
#define __PROXY_H__

/**
 * 反向代理, 端口映射
 * */
namespace rpr{
    struct peer{
        int port;
        const char* addr;
    };

    typedef struct
    {
        int sockfd;
        struct peer peer;
    } r_sock;

    typedef struct{
        int id;
        unsigned int len;
    }Message;

    class Mapper{
        private:
            /**
             * 0: 映射地址
             * 1: 访问地址
             * **/
            struct peer *mapper;

        public:
            Mapper(struct peer *mapper){
                this->mapper = mapper;
            }

            struct peer get(int i){
                return mapper[i];
            }

            virtual void start() = 0;
            virtual void stop() = 0;
    };

    int setnoblocking(int fd);

    extern Mapper* make_server(struct peer[]);
    extern Mapper* make_client(struct peer[]);
    extern int make_tcp(struct peer peer, r_sock *sock);
    extern int make_connect(struct peer peer, r_sock *sock);
}
#endif