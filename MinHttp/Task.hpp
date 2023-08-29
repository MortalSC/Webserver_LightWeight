#pragma once

#include <iostream>
#include <functional>
#include "Protocol.hpp"
class Task
{
private:
    int _sock;
    CallBack _handler; // 回调
public:
    Task() {}
    // Task(int sock, CallBack handler)
    //     : _sock(sock), _handler(handler) {}
    Task(int sock)
        : _sock(sock){}

    // 执行任务
    void ProcessOn()
    {
        _handler(_sock);
    }

    ~Task() {}
};