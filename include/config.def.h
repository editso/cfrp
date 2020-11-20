#define BIND_ADDR "0.0.0.0"
#define LOCAL_ADDR "127.0.0.1"

/**
 * 服务端映射信息
*/
c_peer server_mapper[] = {
    {8080, BIND_ADDR}, // 映射连接地址
    {8888, BIND_ADDR} // 实际访问地址
};

/**
 * 客户端映射信息
*/
c_peer client_mapper[] = {
    {8080, BIND_ADDR}, // 服务连接地址
    {22, LOCAL_ADDR}, // 映射端口
};