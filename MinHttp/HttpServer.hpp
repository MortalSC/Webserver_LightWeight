#pragma once

#include <iostream>
#include <pthread.h>
#include "TcpServer.hpp"
#include "Protocol.hpp"

/* 设置默认端口号 */
#define PORT 8080

class HttpServer
{
public:
    HttpServer(const uint16_t port = PORT)
        : _port(port),_tcp_server(nullptr),_stop(false)     // _stop 标识默认服务不会停止！
    {
    }

    void InitHttpServer(){
        _tcp_server = TcpServer::GetInstance(_port);
    }

    void Loop(){
        /* 获取套接字 */
        int listensock = _tcp_server->GetSock();
        while(!_stop){
            /* 获取网络链接 */
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int sock = accept(listensock, (struct sockaddr*)&peer, &len);
            if(sock < 0){
                // 获取失败！
            }
            /* 重新创建一个拷贝的sock，防止在多线程访问下的数据异常 */
            int *_sock = new int(sock);
            /* 创建线程：执行任务 */
            pthread_t tid;
            pthread_create(&tid, nullptr, Entrence::HandlerRequest, (void*)_sock);
            /* 线程分离：实现无需等待！执行完任务后自动释放资源！ */
            pthread_detach(tid);
        }
    }

private:
    /* 端口号 */
    uint16_t _port;
    /* 网络服务实例 */
    TcpServer* _tcp_server;
    /* 服务是否可停止 */
    bool _stop;
};