#include <iostream>

#ifndef __PROXY_H__
#define __PROXY_H__

/**
 * 反向代理, 端口映射
 * */
namespace cfrp{

    enum{
        CONNECT = 0x01, // 连接
        DISCONNECT = 0x02, // 断开
        EXCEPT = 0x03, // 异常
    };

    /**
     * 数据包信息
    */
    struct cfrp_head{
        /**
         * 类型
         * 1100000 0000000
         * 0x01 
         * 0x02
         * 0x03
         * 校验信息长度
         * 00111111 0000000
         * 本次服务唯一长度
         * 00000000 1111111
         */
        unsigned short mask;
        /**
         * 数据包
        */
        unsigned int size;
    };

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

    extern char* pack(const struct packet*);

    extern struct packet* unpack(char*);


    extern int make_tcp(struct peer peer, r_sock *sock);
    extern int make_connect(struct peer peer, r_sock *sock);

    


}
#endif