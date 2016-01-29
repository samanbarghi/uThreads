/*
 * EchoClient.cpp
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

int main(int argc, char* argv[]){

    if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
      exit(1);
    }
    uint clientPort = atoi(argv[1]);

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    char msg[1024], reply[2048];

    try{
        Connection cconn(AF_INET , SOCK_STREAM , 0);
        if(cconn.connect((struct sockaddr *)&server, sizeof(server)) < 0){
           cerr << "Failed to connect to the server!" << errno << endl;
           return 1;
        }
        for(;;){
            cout << "Enter a message:" ;
            cin >> msg;

            if(cconn.send(msg, strlen(msg), 0) < 0){
                cerr << "Failed to send the message" << endl;
                return 1;
            }

            if(cconn.recv(reply, 2048, MSG_WAITALL) < 0){
               cerr << "Failed to receive the message from the server" << endl;
            }

            cout << "Server says: " << reply << endl;
            std::fill(msg, msg+strlen(msg), 0);
            std::fill(reply, reply+strlen(reply), 0);
        }

    }catch (std::system_error& error){
        std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
    }

    return 0;
}
