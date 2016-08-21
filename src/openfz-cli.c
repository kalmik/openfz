
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/time.h>
#include "logger.h"
#include "request.h"

int main()
{
    struct timeval timeout = {0,200}; // 100ms timeout
    int sockfd;
    int len;
    ssize_t rcv_len;
    struct sockaddr_in address;
    
    char* input;
    struct request_payload response;

    sockfd  = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_port = htons(1337);
    if (inet_aton("127.0.0.1", &address.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval));

    len = sizeof(address);

    rl_bind_key('\t', rl_complete);
    rcv_len = connect(sockfd, (struct sockaddr *) &address, len);

    if (rcv_len == -1)
    {
        perror ("Houve erro no cliente");
        exit(1);
    }
    while (1) {
        prompt();
        input = readline(NULL);
        write(sockfd, input, REQ_BUFFER_SIZE);
        read(sockfd, &response, sizeof(struct request_payload));

        printf("%i %s\n", response.status, response.msg);
        if (strcmp(response.msg, "END") == 0){
            break;
        }
    }
    close(sockfd);

    exit(0);
}
