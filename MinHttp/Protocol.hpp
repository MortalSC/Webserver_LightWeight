#pragma once

#include <iostream>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "Util.hpp"
#include "Log.hpp"

/**
 * 自定义协议 / 任务处理
 */

// #define DEBUG
#define WEB_ROOT "./wwwroot" // 服务器资源指定根目录
#define DEFAUTL_PAGE "index.html"

#define SEP ": "
#define DEFAULT_STATUS_CODE 0
#define OK 200
#define NOT_FOUND 404

/* 请求 */
class HttpRequest
{
public:
    std::string Req_line;              // 记录请求行信息
    std::vector<std::string> Req_head; // 记录请求报头信息
    std::string blank;                 // 空行
    std::string Req_body;              // 记录请求正文

    std::string method;  // 请求方法
    std::string uri;     // 请求的资源：/path ? args
    std::string version; // 请求版本

    int content_length; // 报文中的正文长度！
    std::unordered_map<std::string, std::string> head_kv;

    std::string path;         // 资源路径
    std::string query_string; // 含参url中的参数串

    bool cgi; // cgi 标志位

public:
    HttpRequest() : content_length(OK), cgi(false) {}
    ~HttpRequest() {}
};

/* 响应 */
class HttpResponse
{
public:
    std::string Status_line;            // 记录响应状态行信息
    std::vector<std::string> Resq_head; // 记录响应报头信息
    std::string blank;                  // 空行
    std::string Resq_body;              // 记录响应正文

    int status_code; // 响应状态码
public:
    HttpResponse() : status_code(0) {}
    ~HttpResponse() {}
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
    void RecvHttpRequest()
    {
        GetRequestLine();       // 读取请求行
        GetRequestHead();       // 读取请求报头
        ParseHttpRequestLine(); // 解析请求行
        ParseHttpRequestHead(); // 解析请求报头属性
        RecvHttpRequestBody();  // （若存在）读取请求正文
    }

    // /* 分析 / 解析 请求 */
    // void ParseHttpRequest()
    // {

    // }

    int ProcessNonCIG(){
        return 0;
    }

    /* 构建响应 */
    void BuildHttpResponse()
    {
        // 请求合法性判断！【此处目前只处理 POST 和 GET】
        // 非法则返回特定响应
        auto &code = response.status_code;
        std::string _path;
        if (request.method != "POST" && request.method != "GET")
        {
            // 请求非法
            LogMessage(WARNING, "Requesting method is not right ...");
            code = NOT_FOUND;
            goto END;
        }
        if (request.method == "GET")
        {
            // 对于 get 方法：可以使用 url 进行参数传递，关键点看有没有 ：？【解析：uri】
            size_t pos = request.uri.find('?');
            if (pos != std::string::npos)
            {
                // 存在参数
                // 截取形成 path + args...
                Util::CutString(request.uri, request.path, request.query_string, "?");
                request.cgi = true;
            }
            else
            {
                // 无参数
                request.path = request.uri;
            }
        }
        else if(request.method == "POST")
        {
            // POST
            request.cgi = true;
        }else{
            // 其他请求方式
        }


        // 拼接服务器指定的资源根目录
        _path = request.path;
        request.path = WEB_ROOT;
        request.path += _path;
        // 未指定资源访问的默认处理方式
        if (request.path[request.path.size() - 1] == '/')
        {
            request.path += DEFAUTL_PAGE;
        }

        // 判断路径存在性！
        struct stat st;
        if (stat(request.path.c_str(), &st) == 0)
        {
            // 获取成功！资源存在！
            // 判断是否为目录（未指定访问的资源）
            if (S_ISDIR(st.st_mode))
            {
                request.path += "/";
                request.path += DEFAUTL_PAGE;
            }
            // 访问的是一个文件【如果是一个可执行程序，需要单独处理】
            // 判断文件可执行性
            if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            {
                request.cgi = true;

            }
        }
        else
        {
            // 获取失败！资源不存在！
            std::string info = request.path;
            info += "Not Fount !";
            LogMessage(WARNING, info);
            code = NOT_FOUND;
            goto END;
        }

        if(request.cgi){
            // 以 cgi 的方式处理请求
            // ProcessCIG();
        }else{
            // 非 cgi 的方式就是进行简单的网页文本返回（返回静态页面）
            // ProcessNonCIG();

        }

        std::cout << "Debug : " << request.path << std::endl;
        // LogMessage(INFO, request.uri);
        // LogMessage(INFO, request.path);
        // LogMessage(INFO, request.query_string);
    END:
        return;
    }

    /* 发送响应 */
    void SendHttpResquese() {}

    ~EndPoint() {}

private:
    // 获取请求行信息
    void GetRequestLine()
    {
        // 读取请求行信息
        auto &line = request.Req_line;
        Util::ReadInLine(_sock, request.Req_line);
        line.resize(line.size() - 1); // 去掉尾部的 '\n'
        LogMessage(INFO, request.Req_line);
    }
    // 获取报头信息
    void GetRequestHead()
    {
        // 获取报头内的全部信息
        std::string line;
        while (line != "\n")
        {
            line.clear();
            Util::ReadInLine(_sock, line);
            // 空行的获取
            if (line == "\n")
            {
                request.blank = line;
                break;
            }
            line.resize(line.size() - 1); // 去掉尾部的 '\n'
            request.Req_head.push_back(line);
            // LogMessage(INFO, line);
        }
    }

    // 解析请求行数据
    // 分别获取：请求方法 uri http版本
    void ParseHttpRequestLine()
    {
        auto &line = request.Req_line;
        std::stringstream ss(line);
        ss >> request.method >> request.uri >> request.version;
        auto& method = request.method;
        std::transform(method.begin(), method.end(),method.begin(), ::toupper);
        // LogMessage(INFO, request.method);
        // LogMessage(INFO, request.uri);
        // LogMessage(INFO, request.version);
    }

    // 解析报头属性：使其格式为键值对形式
    void ParseHttpRequestHead()
    {
        std::string key;
        std::string value;
        for (auto &iter : request.Req_head)
        {
            if (Util::CutString(iter, key, value, SEP))
                request.head_kv.insert(std::make_pair(key, value));
        }
    }

    // 判断是否有正文内容：即判断请求是否为：POST 方法
    bool IsNeedRecvHttpRequestBody()
    {
        auto method = request.method;

        if (method == "POST")
        {
            auto &head_kv = request.head_kv;
            auto iter = head_kv.find("Content-Length");
            if (iter != head_kv.end())
            {
                // 找了对应属性
                request.content_length = atoi(iter->second.c_str());
                return true;
            }
        }
        return false;
    }

    // 读取报文中的正文内容
    void RecvHttpRequestBody()
    {
        if (IsNeedRecvHttpRequestBody())
        {
            int content_length = request.content_length;
            auto &body = request.Req_body;
            char ch = 0;
            while (content_length--)
            {
                ssize_t s = recv(_sock, &ch, 1, 0);
                if (s > 0)
                {
                    // 读取成功
                    body.push_back(ch);
                    content_length--;
                }
                else
                {
                    break;
                }
            }
        }
    }

private:
    int _sock;
    HttpRequest request;
    HttpResponse response;
};

/* 请求与响应处理 */
class Entrence
{
public:
    static void *HandlerRequest(void *_sock)
    {
        LogMessage(INFO, "Handler request begin ...");
        int sock = *(int *)_sock;
        delete (int *)_sock;

        /* http请求流程 */
        EndPoint *ep = new EndPoint(sock);
        ep->RecvHttpRequest(); // 接收请求，解析请求
        // ep->ParseHttpRequest();  //
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
        LogMessage(INFO, "Handler request end ...");

        close(sock);
        return nullptr;
    }
};
