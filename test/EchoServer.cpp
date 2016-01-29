/*
 * EchoServer.cpp
 *
 *  Created on: Jan 28, 2016
 *      Author: Saman Barghi
 */


#include "uThreads.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;

void echo(void* c){

     Connection* cconn = (Connection*)c;
     std::vector<char>  msg(1025);
     int res;

     while( (res = cconn->recv(msg.data(), msg.size(), 0)) > 0){
         cconn->write(msg.data(), res);
     }
     if(res == 0){
         cout << "Client closed the connection" << endl;
         cconn->close();
         delete cconn;
     }
     if(res < 0){
         cerr << "Receiving from client failed!" << endl;
         cconn->close();
         delete cconn;
     }
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
      exit(1);
    }
    uint serverPort = atoi(argv[1]);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(serverPort);

    //Creating the socket
    try{
        Connection sconn(AF_INET, SOCK_STREAM , 0);
        sconn.bind((struct sockaddr *) &servaddr, sizeof(servaddr));
        sconn.listen(10);
        for(;;){
            Connection* cconn  = sconn.accept((struct sockaddr*)nullptr, nullptr);
            cout << "Accepted" << endl;
            uThread::create()->start(Cluster::getDefaultCluster(), (void*)echo, (void*)cconn);
        }
    }catch (std::system_error& error){
            std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
    }

    return 0;
}
