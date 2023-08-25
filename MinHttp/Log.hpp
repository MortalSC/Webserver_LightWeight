#pragma once

/**
 * 日志：
 * 处理各种打印或已知错误输出
*/
#include <iostream>
#include <string>
#include <ctime>
/* 日志格式：[日志级别] [时间戳] [日志信息] [错误文件名称] [行数] */

#define INFO    1
#define WARNING 2
#define ERROR   3
#define FATAL   4

#define LogMessage(level, message) log(#level, message, __FILE__, __LINE__)


void log(std::string level, std::string message, std::string file_name, int line){
    printf("[%s] [%d] [%s] [%s] [%d]\n", level.c_str(), time(nullptr), message.c_str(), file_name.c_str(), line);
}