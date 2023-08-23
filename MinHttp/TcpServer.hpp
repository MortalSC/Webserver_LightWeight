#pragma once

#include <iostream>
#include <cstdlib>
#include <cstring>
/* 套接字头文件 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* 线程头文件 */
#include <pthread.h>

/* 定义默认端口号：说明：如果采用云服务器，若对外界开放访问，需要在 安全组 中开放端口号！ */
#define PORT 8080
#define BACKLOG 5

/* 单例化设计 */
class TcpServer
{
private:
    static TcpServer* Tcptr;
private:
    TcpServer(const uint16_t port = PORT)
        : _port(port), _listensock(-1)
    {
    }

    TcpServer(const TcpServer &tcpser){}
public:
    static TcpServer* GetInstance(int port){
        // 加锁保护！
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        if(nullptr == Tcptr){
            // 无实例对象
            pthread_mutex_lock(&lock);
            if(nullptr == Tcptr){
                // 实例化对象
                Tcptr = new TcpServer(port);
                // 启动服务
                Tcptr->InitServer();
            }
            pthread_mutex_unlock(&lock);
        }
        return Tcptr;
    }

    /* 初始化方法 */
    void InitServer(){
        // 三步走：创建、绑定、监听
        Socket();
        Bind();
        Listen();
    }

    /* 创建套接字 */
    void Socket(){
        _listensock = socket(AF_INET, SOCK_STREAM, 0);
        if(_listensock < 0){
            // 创建套接字失败！
            exit(1);
        }
        // 【注】
        /**
         * 当一个连接断开时，主动断开的一方会处于 TIME_WAIT 状态！会在一定时间段内占用 socket！
         * 导致无法使用！
         * 解决方式：设置 允许 复用！
        */
        int opt = 1;
        setsockopt(_listensock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    /* 绑定套接字 */
    void Bind(){
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(_port);
        // 云服务器不能直接绑定公网 IP（ip是特定厂商的虚拟设定）！故设置为：任意绑定
        local.sin_addr.s_addr = INADDR_ANY;     
        
        // 绑定
        if(bind(_listensock, (struct sockaddr*)&local, sizeof(local) < 0)){
            exit(2);
        }
        // 到此为止，绑定成功！
    }

    /* 监听套接字 */
    void Listen(){
        // BACKLOG：全连接长度
        if(listen(_listensock, BACKLOG) < 0){
            exit(3);
        }
        // 到此为止，启动监听（处于监听态）！
    }


    ~TcpServer(){}

private:
    /* 不指定 ip ！原因：当前使用的云服务器！ip地址实际是虚拟ip！ */
    /* 关于 ip 的绑定策略：使用随机绑定！ */

    /* 访问的端口号：_port */
    uint16_t _port;

    /*  */
    int _listensock;
};


TcpServer* TcpServer::Tcptr = nullptr;