#pragma once

#include <iostream>
#include <pthread.h>
#include "TcpServer.hpp"
// #include "Protocol.hpp"
#include "Log.hpp"
/* 信号 */
#include <signal.h>
#include "Task.hpp"
#include "ThreadPool.hpp"

/* 设置默认端口号 */
#define PORT 8080

class HttpServer
{
private:
    /* 端口号 */
    uint16_t _port;
    /* 服务是否可停止 */
    bool _stop;

    // /* 网络服务实例 */
    // TcpServer *_tcp_server;


    // /* 线程池对象 */
    // ThreadPool _threadpool;

public:
    HttpServer(const uint16_t port = PORT)
        : _port(port) /* ,_tcp_server(nullptr)【引入线程池：删】*/ 
        ,_stop(false) // _stop 标识默认服务不会停止！
    {
    }

    void InitHttpServer()
    {
        // 设置忽略管道对端关闭OS给我们的信号【避免该情况导致服务器挂了】
        signal(SIGPIPE, SIG_IGN);
        // _tcp_server = TcpServer::GetInstance(_port);
    }

    void Loop()
    {
        TcpServer *_tcp_server = TcpServer::GetInstance(_port);
        /* 获取套接字 【引入线程池：删】*/
        // int listensock = _tcp_server->GetSock();
        LogMessage(INFO, "TcpServer launch ...");

        while (!_stop)
        {
            /* 获取网络链接 */
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);
            int sock = accept(_tcp_server->GetSock(), (struct sockaddr *)&peer, &len);
            if (sock < 0)
            {
                // 获取失败！
            }
            LogMessage(INFO, "Get a new link ...");

            // 引入线程池
            Task task(sock);
            ThreadPool::GetInstance()->PushTask(task);

            // /* 重新创建一个拷贝的sock，防止在多线程访问下的数据异常 */
            // int *_sock = new int(sock);
            // /* 创建线程：执行任务 */
            // pthread_t tid;
            // pthread_create(&tid, nullptr, Entrence::HandlerRequest, (void *)_sock);
            // /* 线程分离：实现无需等待！执行完任务后自动释放资源！ */
            // pthread_detach(tid);
        }
    }
};