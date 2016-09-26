
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <getopt.h>
#include "logger.h"
#include "request.h"

void sigint_handler (int signo);
unsigned char _exit_ = 0;

int main(int argc, char* argv[])
{
    signal(SIGINT, sigint_handler);
    int sockfd;
    int len;
    ssize_t rcv_len;
    struct sockaddr_in address;
    
    char* input;
    char log[LOGGER_BUFFER_SIZE];
    char request[REQ_BUFFER_SIZE];
    char response[REQ_BUFFER_SIZE];

    int opt;
    char host[15] = "127.0.0.1";

    while ((opt = getopt(argc, argv, "h:")) != -1) {
        switch (opt) {
            case 'h':
                sprintf(host, optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-h host]\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    sockfd  = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_port = htons(1337);
    if (inet_aton(host, &address.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    len = sizeof(address);

    rl_bind_key('\t', rl_complete);
    rcv_len = connect(sockfd, (struct sockaddr *) &address, len);

    if (rcv_len == -1)
    {
        perror("Connection Error!");
        exit(1);
    }

    while (!_exit_) {
        prompt();
        input = readline(NULL);
        if(strcmp(input, "")==0) continue;
        sprintf(request, input);
        sprintf(log, "%s\n", request);
        logger(INFO, log);
        send(sockfd, request, REQ_BUFFER_SIZE, 0);
        recv(sockfd, response, REQ_BUFFER_SIZE, 0);
        sprintf(log, "%s\n", response);
        logger(INFO, log);
        if (strcmp(response, "END") == 0){
            _exit_ = 1;
            break;
        }

        add_history(input);
    }

    close(sockfd);

    exit(0);
}
void sigint_handler (int signo) {
    logger(WARN, "To exit type shutdown");
    prompt();
}
