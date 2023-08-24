#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

// 工具类

class Util
{
public:
    /* 对报头信息按行读取 */
    /**
     * 参数：
     *  std::string &out：输出型参数，向外返回解析出来的内容
     */
    static int ReadInLine(int sock, std::string &out)
    {

        // 对报文进行按行读取，设计方法兼容各种换行风格
        char ch = 'X';
        while (ch != '\n')
        {
            // 按字节方式读取
            ssize_t s = recv(sock, &ch, 1, 0);
            if (s > 0)
            {
                // 读到数据
                if (ch == '\r')
                {
                    // 处理按 '\r' or '\r\n' 作为换行符的情况，
                    // 操作我们需要对 \r 后的内容进行判断
                    /*
                         一般情况下，recv 是读取并取走缓冲区中的数据
                         对于：'\r\n'，是正常情况
                         对于：'\r' 就造成了数据读取走而丢失有效数据情况
                         解决方式：recv 的 MSG_PEEK 模式参数（窥探：只读不取走数据）
                    */
                    recv(sock, &ch, 1, MSG_PEEK);
                    if (ch == '\n')
                    {
                        // 说明：换行符是 '\r\n'
                        recv(sock, &ch, 1, 0);
                    }
                    else
                    {
                        // 说明：换行符是 '\r'
                        ch = '\n';
                    }
                }
                // 到此：只有 \n or 普通字符
                out.push_back(ch);
            }
            else if (s == 0)
            {
                // 读到结尾
                return 0;
            }
            else
            {
                // 读取错误
                return -1;
            }
        }
        return out.size();
    }
};