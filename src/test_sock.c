#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "mod-fis.h"

int main( )
{
    int _exit_ = 0;
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;

    double* request;
    double* response;
    request = (double*) malloc(sizeof(double)*2);
    response = (double*) malloc(sizeof(double)*1);

    sockfd  = socket(AF_INET, SOCK_STREAM,0);  // criacao do socket

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("192.168.0.26");
    address.sin_port = 3000;

    len = sizeof(address);

    result = connect(sockfd, (struct sockaddr *) &address, len);

    if (result == -1) {
        perror ("Houve erro no cliente");
        exit(1);
    }


    srand((unsigned int)time(NULL));
    while(!_exit_) {
        request[0] = rand()%1001/1000.0;
        request[1] = rand()%1001/1000.0;

        send(sockfd, request, 2* sizeof(double),0);
        recv(sockfd, response, 2* sizeof(double), 0);

        usleep(100000);
    }

    close(sockfd);
    exit(0);
}