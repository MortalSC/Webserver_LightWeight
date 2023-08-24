#include <iostream>
#include <string>
#include <memory> // 引入智能指针管理！
// #include "TcpServer.hpp"
#include "HttpServer.hpp"

static void Usage(std::string proc)
{
    std::cout << "Usage =>\t" << proc << " port" << std::endl;
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        Usage(argv[0]);
        exit(4);
    }
    /* 获取用户指定的端口号 */
    uint16_t port = atoi(argv[1]);

    /* version 2 : testing HttpServer.hpp create a task */
    std::shared_ptr<HttpServer> http_server(new HttpServer(port));
    /* 初始化服务 */
    http_server->InitHttpServer();
    /* 启动服务 */
    http_server->Loop();

/* version 1 : testing TcpServer.hpp create a server */
#if 0
    TcpServer* Tcptr = TcpServer::GetInstance(port);
#endif
    while (true)
    {
    }

    return 0;
}