
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "logger.h"

void* logger_run (void* arg);

int main()
{
    int sockfd;
    int len;
    struct sockaddr_un address;
    int result;
    
    char* input;
    

    pthread_t logger_handler;

    sockfd  = socket(AF_UNIX, SOCK_STREAM,0);

    address.sun_family = AF_UNIX;
    strcpy (address.sun_path, "openfz.sock");
    len = sizeof(address);

    result = connect(sockfd, (struct sockaddr *) &address, len);

    if (result == -1) {
        printf("ERRO\n");
        exit(1);
    }

    pthread_create(&logger_handler, NULL, logger_run, NULL);
    rl_bind_key('\t', rl_complete);
    while (1) {
        input = readline(NULL);
        write(sockfd, input , LOGGER_BUFFER_SIZE);
        
    }

    pthread_join(logger_handler, NULL);

    close(sockfd);

    exit(0);
}

void* logger_run (void* arg) 
{
    int sock_in_fd;
    char log[LOGGER_BUFFER_SIZE];

    sock_in_fd = open("openfz.log", O_RDONLY);

    while (1) {
        read(sock_in_fd, log, LOGGER_BUFFER_SIZE);
        printf("%s\n", log);
    }
}
