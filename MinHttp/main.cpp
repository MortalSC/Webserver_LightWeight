#include <iostream>
#include <string>
#include "TcpServer.hpp"

static void Usage(std::string proc){
    std::cout << "Usage =>\t" << proc << " port" << std::endl;
}

int main(int argc, char* argv[]){

    if(argc != 2){
        Usage(argv[0]);
        exit(4);
    }

    uint16_t port = atoi(argv[1]);
    TcpServer* Tcptr = TcpServer::GetInstance(port);

    return 0;
}