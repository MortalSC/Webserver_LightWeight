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
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "Util.hpp"
#include "Log.hpp"

/**
 * 自定义协议 / 任务处理
 */

// #define DEBUG
#define HTTP_VERSION "HTTP/1.0"
#define WEB_ROOT "./wwwroot" // 服务器资源指定根目录
#define DEFAUTL_PAGE "index.html"
#define LINE_END "\r\n"
#define PAGE_404 "404.html"

#define SEP ": "
#define DEFAULT_STATUS_CODE 0
#define OK 200
#define BAD_REQUSET 400
#define NOT_FOUND 404
#define SERVER_ERROR 500

static std::string Code2Desc(int code)
{
    std::string desc;
    switch (code)
    {
    case 200:
        desc = "OK";
        break;
    case 404:
        desc = "Not Found";
        break;
    default:
        break;
    }
    return desc;
}

static std::string Suffix2Desc(const std::string &suffix)
{
    static std::unordered_map<std::string, std::string> suffix2desc = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/x-javascript"},
        {".jpg", "image/jpeg"},
        {".xml", "application/xml"}};
    auto iter = suffix2desc.find(suffix);
    if (iter != suffix2desc.end())
    {
        // 找到了
        return iter->second;
    }
    return "text/html";
}

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
    int size;
    std::string suffix; // 资源后缀名

public:
    HttpRequest() : content_length(0), cgi(false) {}
    ~HttpRequest() {}
};

/* 响应 */
class HttpResponse
{
public:
    std::string Status_line;            // 记录响应状态行信息
    std::vector<std::string> Resp_head; // 记录响应报头信息
    std::string blank;                  // 空行
    std::string Resp_body;              // 记录响应正文

    int Status_code; // 响应状态码

    int fd; // 记录暂时打开的读取目标文件（资源）
    // int filesize; // 被访问的目标资源（文件）大小
public:
    HttpResponse() : blank(LINE_END), Status_code(OK), fd(-1) {}
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
        : _sock(sock), stop(false)
    {
    }

    bool IsStop() { return stop; }

    /* 读取请求 */
    void RecvHttpRequest()
    {
        // || 逻辑或具有短路请求的作用！
        // if (GetRequestLine() || GetRequestHead())
        if ((!GetRequestLine()) && (!GetRequestHead()))
        {
            // 进入到这说明读取没有出错！【否则就是错误】
            ParseHttpRequestLine(); // 解析请求行
            ParseHttpRequestHead(); // 解析请求报头属性
            RecvHttpRequestBody();  // （若存在）读取请求正文
        }
    }

    // 构建响应
    int ProcessNonCIG()
    {
        // 响应的正文内容提取方式
        // 从一个文件描述符中把数据考到另一个中
        // 打开指定的资源文件
        response.fd = open(request.path.c_str(), O_RDONLY);
        // 当且仅当文件是有效的才进行构建响应

        if (response.fd >= 0)
        {
#if 0
            // // 构建响应行
            // response.Status_line = HTTP_VERSION;
            // response.Status_line += " ";
            // response.Status_line += std::to_string(response.Status_code);
            // response.Status_line += " ";
            // response.Status_line += Code2Desc(response.Status_code);
            // response.Status_line += LINE_END;

            // // 构建响应报头【主要是：正文大小和正文类型】

            // std::string header_line = "Content-Type: ";
            // header_line += Suffix2Desc(request.suffix);
            // header_line += LINE_END;
            // response.Resp_head.push_back(header_line);

            // header_line = "Content-Length: ";
            // header_line += std::to_string(response.filesize);
            // header_line += LINE_END;
            // response.Resp_head.push_back(header_line);
#endif
            return OK;
        }
        return NOT_FOUND;
    }

    // 关联 CGI 程序处理数据
    int ProcessCIG()
    {
        // 获取可执行程序
        auto &bin = request.path; // 需要子进程执行的目标程序！一定存在！（前面的操作已经保证！）
        int code = OK;
        // 数据来源
        // std::cout << request.Req_body << std::endl;
        /* 修复bug过程的输出 */
        // LogMessage(INFO, request.Req_body);

        auto &method = request.method;
        auto &query_string = request.query_string; // GET：
        auto &body_text = request.Req_body;        // POST：
        int content_length = request.content_length;
        auto &response_body = response.Resp_body;

        // 对于Get方法可以使用环境变量的方式传递数据
        // 环境变量是具有全局属性的(可以被子进程继承下去)；不受exec*程序替换的影响!
        std::string query_string_env;
        std::string method_env;
        std::string content_length_env;

        // 使用管道通信：约定站在父进程角度！
        int input[2];  // 子 => 父：新数据
        int output[2]; // 父 => 子：原始数据

        if (pipe(input) < 0)
        {
            // 创建失败
            LogMessage(ERROR, "pipe input error ...");
            code = SERVER_ERROR;
            return code;
        }
        if (pipe(output) < 0)
        {
            // 创建失败
            LogMessage(ERROR, "pipe output error ...");
            code = SERVER_ERROR;
            return code;
        }

        // 在被调用是实际走到这得就是一个新线程！【从头到尾都只有一个进程！（httpserver）】
        // 目的调用：CGI程序！
        pid_t pid = fork();
        if (pid > 0)
        {
            // 1. 通信信道建立
            // 父进程
            // 父进程把数据给子进程（output），关闭读
            // 父进程从子进程获取返回数据（input），关闭写
            close(output[0]);
            close(input[1]);
            std::cout << std::endl;
            /* 修复bug过程的输出 */
            // LogMessage(INFO, request.Req_body);
            // LogMessage(INFO, body_text);

            if (method == "POST")
            {
                /* 修复bug过程的输出 */
                // LogMessage(INFO, request.Req_body);
                // LogMessage(INFO, body_text);
                const char *start = body_text.c_str();
                int total = 0;
                int size = 0;
                std::cout << *start << std::endl;
                std::cout << body_text << std::endl;
                /* 修复bug过程的输出 */
                // LogMessage(INFO, request.Req_body);
                // LogMessage(INFO, body_text);

                while (total < content_length && (size = write(output[1], start + total, body_text.size() - total)) > 0)
                {
                    total += size;
                }
                std::cout << body_text << std::endl;
            }

            // 父进程获取子进程的处理结果！
            char ch = 0;
            while (read(input[0], &ch, 1) > 0)
            {
                response_body.push_back(ch);
            }

            // 检测子进程是否成功退出
            int status = 0;
            pid_t ret = waitpid(pid, &status, 0);
            if (ret == pid)
            {
                // 等待成功！
                if (WIFEXITED(status))
                {
                    if (WEXITSTATUS(status) == 0)
                    {
                        code = OK;
                    }
                    else
                    {
                        code = BAD_REQUSET;
                    }
                }
                else
                {
                    code = SERVER_ERROR;
                }
            }

            close(input[0]);
            close(output[1]);
        }
        else if (pid == 0)
        {
            // 1. 通信信道建立
            // 子进程：用于调用CGI程序处理数据
            // 父进程把数据给子进程（output），关闭读【子进程关闭写】
            // 父进程从子进程获取返回数据（input），关闭写【子进程关闭读】
            close(output[1]);
            close(input[0]);

            // 难点：程序替换后！文件描述符的值就不知道了！（数据替换了）但是曾经打开的管道文件依旧存在！
            // 程序替换值替换代码和数据，不会替换内核相关的数据结构（包括文件描述符表）
            // 解决方式：设定约定：让目标被替换后的进程，读取管道等价于读取标准输入、写入管道等价于写到标准输出
            // 这些需要在程序替换前作重定向操作！！！

            method_env = "METHOD=";
            method_env += method;
            putenv((char *)method_env.c_str());

            if ("GET" == method)
            {
                query_string_env = "QUERY_STRING=";
                query_string_env += query_string;
                putenv((char *)query_string_env.c_str());
            }
            else if (method == "POST")
            {
                content_length_env = "CONTENT_LENGTH=";
                content_length_env += std::to_string(content_length);
                putenv((char *)content_length_env.c_str());
                LogMessage(INFO, "POST METHOD, ADD CONTENT_LENGTH");
            }
            else
            {
                // Nothing to do
            }

            // 子进程角度
            // input[1]：写入数据
            // output[0]：读取数据（从父进程来）
            dup2(input[1], 1);  // 子进程向外输出
            dup2(output[0], 0); // 向子进程输入

            execl(bin.c_str(), bin.c_str(), nullptr);
            exit(1); // 如果失败
        }
        else
        {
            // 失败！
            LogMessage(ERROR, "fork error");
            return 404;
        }
        return code;
    }

    /* 构建响应(判断请求调用构造响应) */
    void BuildHttpResponse()
    {

        // 到此请求已经处理完成！实际就可以构建响应了！【该函数主要处理的关于响应正文内容产生问题】

        // 请求合法性判断！【此处目前只处理 POST 和 GET】
        // 非法则返回特定响应
        auto &code = response.Status_code;
        std::string _path;
        // 判断路径存在性！
        struct stat st; //
        int size = 0;
        std::size_t found = 0;
        if (request.method != "POST" && request.method != "GET")
        {
            // 请求非法
            LogMessage(WARNING, "Requesting method is not right ...");
            code = BAD_REQUSET;
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
        else if (request.method == "POST")
        {
            // POST
            request.cgi = true;
            request.path = request.uri;
        }
        else
        {
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

        if (stat(request.path.c_str(), &st) == 0) // 此处获取不一定成功
        {
            // 获取成功！资源存在！
            // 判断是否为目录（未指定访问的资源）
            if (S_ISDIR(st.st_mode))
            {
                request.path += "/";
                request.path += DEFAUTL_PAGE;
                stat(request.path.c_str(), &st); // 再次获取 【有效】请求资源的信息！
            }
            // 访问的是一个文件【如果是一个可执行程序，需要单独处理】
            // 判断文件可执行性
            if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
            {
                request.cgi = true;
            }
            request.size = st.st_size; // 记录文件大小（被访问资源）
        }
        else
        {
            // 获取失败！资源不存在！
            LogMessage(WARNING, request.path + " Not Found");
            code = NOT_FOUND;
            goto END;
        }

        // 判断资源后缀！
        found = request.path.rfind(".");
        if (found == std::string::npos)
        {
            request.suffix = ".html";
        }
        else
        {
            request.suffix = request.path.substr(found);
        }

        if (request.cgi)
        {
            // 以 cgi 的方式处理请求
            code = ProcessCIG(); // 执行目标程序，拿到结果放置在response.Resp_body
        }
        else
        {
            // 到此位置目标网页一定存在！
            // 返回的不单单是网页，还有构建响应！
            // 非 cgi 的方式就是进行简单的网页文本返回（返回静态页面）
            code = ProcessNonCIG(); // 只需要返回静态网页即可
        }

        // LogMessage(INFO, request.uri);
        // LogMessage(INFO, request.path);
        // LogMessage(INFO, request.query_string);
    END:
        // std::cout << "Debug : " << request.path << std::endl;
        BuildHttpResponseHelper(); // 完成状态行和响应报头的填充！

        return;
    }

    /* 发送响应 */
    void SendHttpResquese()
    {
        // 底层实际是把数据拷贝到发送缓冲区中！
        // 发送响应行信息
        send(_sock, response.Status_line.c_str(), response.Status_line.size(), 0);
        for (auto iter : response.Resp_head)
        {
            send(_sock, iter.c_str(), iter.size(), 0);
        }
        send(_sock, response.blank.c_str(), response.blank.size(), 0);
        if (request.cgi)
        {
            auto &response_body = response.Resp_body;
            size_t size = 0;
            size_t total = 0;
            const char *start = response_body.c_str();
            while (total < response_body.size() && (size = send(_sock, start + total, response_body.size() - total, 0) > 0))
            {
                total += size;
            }
        }
        else
        {
            sendfile(_sock, response.fd, nullptr, request.size);
            close(response.fd);
        }
    }

    ~EndPoint() {}

private:
    // 获取请求行信息
    bool GetRequestLine()
    {
        // 读取请求行信息
        auto &line = request.Req_line;
        if (Util::ReadInLine(_sock, request.Req_line) > 0)
        {
            // 正确解析请求行信息才进行后续操作！
            line.resize(line.size() - 1); // 去掉尾部的 '\n'
            LogMessage(INFO, request.Req_line);
        }
        else
        {
            stop = true;
        }
    }
    // 获取报头信息
    bool GetRequestHead()
    {
        // 获取报头内的全部信息
        std::string line;
        while (line != "\n")
        {
            line.clear();
            if (Util::ReadInLine(_sock, line) <= 0)
            {
                // 到此处说明对方发送的数据出错了
                stop = true;
                break;
            }
            // 空行的获取
            if (line == "\n")
            {
                request.blank = line;
                break;
            }
            line.resize(line.size() - 1); // 去掉尾部的 '\n'
            request.Req_head.push_back(line);
            LogMessage(INFO, line);
        }
        return stop;
    }

    // 解析请求行数据
    // 分别获取：请求方法 uri http版本
    void ParseHttpRequestLine()
    {
        auto &line = request.Req_line;
        std::stringstream ss(line);
        ss >> request.method >> request.uri >> request.version;
        auto &method = request.method;
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
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
    bool RecvHttpRequestBody()
    {
        if (IsNeedRecvHttpRequestBody())
        {
            int content_length = request.content_length;
            auto &body = request.Req_body;
            /* 修复bug过程的输出 */
            // LogMessage(INFO, request.Req_body);
            // LogMessage(INFO, std::to_string(content_length));

            char ch = 0;
            while (content_length)
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
                    stop = true;
                    break;
                }
            }
            // LogMessage(INFO, body);
        }
        return stop;
    }

    // 构建响应（出错处理）
    void BuildHttpResponseHelper()
    {
        // 获取状态行内容
        auto &code = response.Status_code;

        // 构建状态行！
        auto &status_line = response.Status_line;
        status_line += HTTP_VERSION;
        status_line += " ";
        status_line += std::to_string(code);
        status_line += " ";
        status_line += Code2Desc(code);
        status_line += LINE_END;

        std::string page = WEB_ROOT;
        page += "/";
        // 响应正文（可能含有报头）
        switch (code)
        {
        case OK:
            BuildOkResponse();
            break;
        case NOT_FOUND:
            page += PAGE_404;
            HandlerError(page);
            break;
        case SERVER_ERROR:
            page += PAGE_404;
            HandlerError(page);
            break;
        case BAD_REQUSET:
            page += PAGE_404;
            HandlerError(page);
            break;
        default:
            break;
        }
    }

    // 200
    void BuildOkResponse()
    {
        std::string line = "Content-Type: ";
        line += Suffix2Desc(request.suffix);
        line += LINE_END;
        response.Resp_head.push_back(line);

        line = "Content-Length: ";
        if (request.cgi)
        {
            line += std::to_string(response.Resp_body.size()); //
        }
        else
        {
            line += std::to_string(request.size); //
        }
        line += LINE_END;
        response.Resp_head.push_back(line);
    }

    // 404
    void HandlerError(std::string page)
    {
        request.cgi = false;
        // 返回对应的404页面！
        response.fd = open(page.c_str(), O_RDONLY);
        if (response.fd > 0)
        {
            struct stat st;
            stat(page.c_str(), &st);
            request.size = st.st_size;

            std::string line = "Content-Type: text/html";
            line += LINE_END;
            response.Resp_head.push_back(line);
            line = "Content-Length: ";
            line += std::to_string(st.st_size);
            line += LINE_END;
            response.Resp_head.push_back(line);
        }
    }

private:
    int _sock;
    HttpRequest request;
    HttpResponse response;

    bool stop; // 用于鉴别是否输入错误等问题
};

/* 请求与响应处理 */
class CallBack
{
public:
    CallBack() {}

    void operator()(int sock){
        HandlerRequest(sock);
    }

    void HandlerRequest(int sock)
    {
        /* http请求流程 */
        EndPoint *ep = new EndPoint(sock);
        ep->RecvHttpRequest(); // 接收请求，解析请求
                               // ep->ParseHttpRequest();  //
        if (!ep->IsStop())
        {
            LogMessage(INFO, "Recv No Error, Begin Build And Send ...");
            ep->BuildHttpResponse(); // 构建响应
            ep->SendHttpResquese();  // 发送响应
        }
        else
        {
            LogMessage(WARNING, "Recv Error ,Delete Build And Send ...");
        }

        LogMessage(INFO, "Handler request end ...");
        delete ep;

        close(sock);
    }
};

#if 0
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
        if (!ep->IsStop())
        {
            LogMessage(INFO, "Recv No Error, Begin Build And Send ...");
            ep->BuildHttpResponse(); // 构建响应
            ep->SendHttpResquese();  // 发送响应
        }
        else
        {
            LogMessage(WARNING, "Recv Error ,Delete Build And Send ...");
        }
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
#endif