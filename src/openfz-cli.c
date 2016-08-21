
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "logger.h"
#include "request.h"

int main()
{
    int sockfd;
    int len;
    struct sockaddr_in address;
    
    char* input;
    struct request_payload response;

    sockfd  = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    address.sin_family = AF_INET;
    address.sin_port = htons(1337);
    if (inet_aton("127.0.0.1", &address.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    len = sizeof(address);

    rl_bind_key('\t', rl_complete);

    while (1) {
        prompt();
        input = readline(NULL);
        sendto(sockfd, input, LOGGER_BUFFER_SIZE, 0, (struct sockaddr *) &address, sizeof(struct sockaddr));
        recvfrom(sockfd, &response, sizeof(struct request_payload), 0, (struct sockaddr *) &address, &len);
        printf("%i %s\n", response.status, response.msg);
        if (strcmp(response.msg, "END") == 0){
            break;
        }
        
    }
    close(sockfd);

    exit(0);
}
