#include <uThreads/uThreads.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "include/http_parser.h"

#define PORT 8800
#define INPUT_BUFFER_LENGTH 4*1024 //4 KB
#define MAXIMUM_THREADS_PER_CLUSTER 8

/* HTTP responses*/
#define RESPONSE_METHOD_NOT_ALLOWED "HTTP/1.1 405 Method Not Allowed\r\n"
#define RESPONSE_NOT_FOUND "HTTP/1.1 404 Not Found\n" \
                            "Content-type: text/html\n" \
                            "\n" \
                            "<html>\n" \
                            " <body>\n" \
                            "  <h1>Not Found</h1>\n" \
                            "  <p>The requested URL was not found on this server.</p>\n" \
                            " </body>\n" \
                            "</html>\n"

// To avoid file io, only return a simple HelloWorld!
#define RESPONSE_OK "HTTP/1.1 200 OK\r\n" \
                    "Content-Length: 21\r\n" \
                    "Content-Type: text/html\r\n" \
                    "Connection: keep-alive\r\n" \
                    "Server: uThreads-http\r\n" \
                    "\r\n" \
                    "<p>Hello World!</p>\n"

/* Logging */
#define LOG(msg) puts(msg);
#define LOGF(fmt, params...) printf(fmt "\n", params);
#define LOG_ERROR(msg) perror(msg);

Connection* sconn; //Server socket

http_parser_settings settings;

typedef struct {
    Connection* conn;
    bool keep_alive;
    char* url;
    int url_length;
} custom_data_t;

ssize_t read_http_request(Connection& cconn, void *vptr, size_t n){

	size_t nleft;
	ssize_t nread;
	char * ptr;

	ptr = (char *)vptr;
	nleft = n;

	uThread::yield(); //yield before read
    while(nleft >0){
    	if( (nread = cconn.recv( ptr, INPUT_BUFFER_LENGTH - 1, 0)) <0){
    		if (errno == EINTR)
    			nread =0;
    		else
    			return (-1);
    	}else if(nread ==0)
    		break;

    	nleft -= nread;

    	//If we are at the end of the http_request
    	if(ptr[nread-1] == '\n' && ptr[nread-2] == '\r' && ptr[nread-3] == '\n' && ptr[nread-4] == '\r')
    		break;

    	ptr += nread;

    }

    return (n-nleft);
}

ssize_t writen(Connection& cconn, const void *vptr, size_t n){

	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = (char*)vptr;
	nleft = n;

	while(nleft > 0){
		if( (nwritten = cconn.send(ptr, nleft, 0)) <= 0){
			if(errno == EINTR)
				nwritten = 0; /* If interrupted system call => call the write again */
			else
				return (-1);
		}
		nleft -= nwritten;
		ptr += nwritten;

	}

	return (n);
}
int on_headers_complete(http_parser* parser){

    //Check whether we should keep-alive or close
    custom_data_t *my_data = (custom_data_t*)parser->data;
    my_data->keep_alive = http_should_keep_alive(parser);

    return 0;
}
int on_header_field(http_parser* parser, const char* header, long unsigned int size){
    printf("%s\n", header);
    return 0;
}

void intHandler(int sig){
    sconn->close();
	exit(1);
}

/* handle connection after accept */
void *handle_connection(void *arg){

	Connection* cconn= (Connection*) arg;

	custom_data_t *my_data = (custom_data_t*)malloc(sizeof(custom_data_t));
	my_data->conn = cconn;
	my_data->keep_alive = 0;
	my_data->url = nullptr;

    http_parser *parser = (http_parser *) malloc(sizeof(http_parser));
    if(parser == nullptr)
        exit(1);
    http_parser_init(parser, HTTP_REQUEST);
	//pass connection and custom data to the parser
    parser->data = (void *) my_data;



    char buffer[INPUT_BUFFER_LENGTH]; //read buffer from the socket
    size_t nparsed;
    ssize_t nrecvd; //return value for for the read() and write() calls.

    do{

        bzero(buffer, INPUT_BUFFER_LENGTH);
        //Since we only accept GET, just try to read INPUT_BUFFER_LENGTH
        nrecvd = read_http_request(*cconn, buffer, INPUT_BUFFER_LENGTH -1);
        if(nrecvd<0){
            //if RST packet by browser, just close the connection
            //no need to show an error.
            if(errno != ECONNRESET){
                LOG_ERROR("Error reading from socket");
                printf("fd %d\n", cconn->getFd());
            }
            break;
        }

        nparsed = http_parser_execute(parser, &settings, buffer, nrecvd);
        if(nrecvd == 0) break;
        if(nparsed != nrecvd){
            LOG_ERROR("Erorr in Parsing the request!");
        }else{
            //We only handle GET Requests
            if(parser->method == 1)
            {
                //Write the response
                writen(*cconn, RESPONSE_OK, sizeof(RESPONSE_OK));
            }else{
                //Method is not allowed
                writen(*cconn, RESPONSE_METHOD_NOT_ALLOWED, sizeof(RESPONSE_METHOD_NOT_ALLOWED));
            }
        }
        //reset data
        my_data->url_length =0;
    }while(my_data->keep_alive);

    cconn->close();
    free(my_data);
    delete cconn;
    free(parser);
}

int main(int argc, char* argv[]) {
    if ( argc != 2 ){
       printf( "USAGE: %s NUMBER_OF_THREADS\n", argv[0] );
       return 0;
    }
    //set total number of worker threads
    size_t thread_count = atoi(argv[1]);
    //Create clusters based on MAXIMUM_THREADS_PER_CLUSTER
    size_t cluster_count = (thread_count/(MAXIMUM_THREADS_PER_CLUSTER+1))+1;

    struct sockaddr_in serv_addr; //structure containing an internet address
    bzero((char*) &serv_addr, sizeof(serv_addr));

    Cluster& defaultCluster = Cluster::getDefaultCluster();
    //Create kThreads, default thread is already started --> i=1
    kThread* kThreads[thread_count-1];
    for(size_t i=1; i < thread_count-1; i++)
        kThreads[i] = new kThread(defaultCluster);


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    //handle SIGINT to close the main socket
    signal(SIGINT, intHandler);

    settings.on_headers_complete = on_headers_complete;
    try{
        sconn = new Connection(AF_INET, SOCK_STREAM, 0);

        if(sconn->bind((struct sockaddr *) &serv_addr, sizeof(serv_addr))<0) {
            LOG_ERROR("Error on binding");
            exit(1);
        };
        sconn->listen(65535);
        while(true){
                Connection* cconn  = sconn->accept((struct sockaddr*)nullptr, nullptr);
                uThread::create()->start(defaultCluster, (void*)handle_connection, (void*)cconn);
        }
        sconn->close();
    }catch(std::system_error& error){
        std::cout << "Error: " << error.code() << " - " << error.what() << '\n';
        sconn->close();
    }

   for(kThread* kt: kThreads) delete kt;
    return 0;
}
