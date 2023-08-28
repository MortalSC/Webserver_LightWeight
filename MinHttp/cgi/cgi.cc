#include <iostream>
#include <cstdlib>
#include <string>
#include <unistd.h>
using namespace std;

int main()
{
    cerr << "....................................................." << endl;
    cerr << "====================>【method】 " << getenv("METHOD") << endl;
    std::string method = getenv("METHOD");
    std::string query_string;
    if ("GET" == method)
    {
        query_string = getenv("QUERY_STRING");
        cerr << "====================>【args】 " << query_string << endl;
    }
    else if ("POST" == method)
    {
        cerr << "====================>【content-length】 " << getenv("CONTENT_LENGTH") << endl;
        int content_length = atoi( getenv("CONTENT_LENGTH"));
        char ch = 0;
        while(content_length){
            read(0, &ch, 1);
            query_string.push_back(ch);
            content_length--;
        }

        cerr << "Get text : " << query_string << endl;
    }
    else
    {
    }
    cerr << "*****************************************************" << endl;

    // 业务型数据处理！

    return 0;
}