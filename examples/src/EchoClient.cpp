/*
 * EchoClient.cpp
 *
 *  Created on: Jan 28, 2016
 *      Author: Saman Barghi
 */

#include <uThreads/uThreads.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <algorithm>

using namespace std;
using namespace uThreads::runtime;
using namespace uThreads::io;

int main(int argc, char* argv[]){

    if (argc != 2) {
      cerr << "Usage: " << argv[0] << " <Server Port>" << endl;
      exit(1);
    }
    uint clientPort = atoi(argv[1]);

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(clientPort );

    std::string msg;
    std::vector<char>  reply(1025);

    int res, count = 0;

    try{
        Connection cconn(AF_INET , SOCK_STREAM , 0);
        if(cconn.connect((struct sockaddr *)&server, sizeof(server)) < 0){
           cerr << "Failed to connect to the server! : " << errno << endl;
           return 1;
        }
        for(;;){

            count = 0;
            cout << "Enter a message (type EXIT to exit):" << endl ;
            getline(cin, msg);
            if(msg.length() == 0)
                continue;
            if(msg.compare("EXIT") == 0)
            {
                cconn.close();
                return 0;
            }


            if(cconn.send(msg.c_str(), msg.length(), 0) < 0){
                cerr << "Failed to send the message" << endl;
                return 1;
            }

            while( (res = cconn.recv(reply.data(), reply.size(), 0)) > 0){
                for(auto i = reply.begin(); i < reply.begin()+res; i++) cout << *i;
                count += res;

                if(count >= msg.length())
                    break;
            }
            if(res ==0){
                cerr << "Server Closed the connection !" << endl;
                cconn.close();
               return 1;

            }
            if(res < 0){
               cerr << "Failed to receive the message from the server: " << errno << endl;
               cconn.close();
               return 1;
            }

            cout << endl;
        }

    }catch (std::system_error& error){
        std::cout << "Error: " << error.code() << " - " << error.what() << endl;
    }

    return 0;
}
