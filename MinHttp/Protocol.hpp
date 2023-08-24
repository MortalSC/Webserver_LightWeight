#pragma once

#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include "Util.hpp"

/**
 * 自定义协议 / 任务处理
 */

// #define DEBUG

/* 请求 */
class HttpRequest
{
public:
    std::string Req_line;              // 记录请求行信息
    std::vector<std::string> Req_head; // 记录请求报头信息
    std::string blank;                 // 空行
    std::string Req_body;              // 记录请求正文
};

/* 响应 */
class HttpResponse
{
public:
    std::string Status_line;            // 记录响应状态行信息
    std::vector<std::string> Resq_head; // 记录响应报头信息
    std::string blank;                  // 空行
    std::string Resq_body;              // 记录响应正文
};

/*
    读取请求，分析请求，构建响应
    IO 通信
*/
class EndPoint
{
public:
    EndPoint(int sock)
        : _sock(sock)
    {
    }

    /* 读取请求 */
    void RecvHttpRequest() {}

    /* 分析 / 解析 请求 */
    void ParseHttpRequest() {}

    /* 构建响应 */
    void BuildHttpResponse() {}

    /* 发送响应 */
    void SendHttpResquese() {}

    ~EndPoint() {}

private:
    // 获取请求行信息
    void GetRequestLine()
    {
        Util::ReadInLine(_sock, request.Req_line);
    }
    // 获取报头信息
    void GetRequestHead(){
        
    }

private:
    int _sock;
    HttpRequest request;
    HttpResponse response;
};

class Entrence
{
public:
    static void *HandlerRequest(void *_sock)
    {
        int sock = *(int *)_sock;
        delete (int *)_sock;

        /* http请求流程 */
        EndPoint *ep = new EndPoint(sock);
        ep->RecvHttpRequest();   // 接收请求
        ep->ParseHttpRequest();  // 分析请求
        ep->BuildHttpResponse(); // 构建响应
        ep->SendHttpResquese();  // 发送响应
        delete ep;

#ifdef DEBUG
        std::cout << "Get a new link ... [ sock : " << sock << "]" << std::endl;

        /* 按行读取数据测试 */
        std::string line;
        Util::ReadInLine(sock, line);
        std::cout << line << std::endl;

        /* 测试获取请求内容 */
        char buffer[4096];
        std::cout << "========================== begin ================================================" << std::endl;
        recv(sock, buffer, sizeof(buffer), 0);
        std::cout << buffer << std::endl;
        std::cout << "========================== end ================================================" << std::endl;

#endif

        close(sock);
        return nullptr;
    }
};
