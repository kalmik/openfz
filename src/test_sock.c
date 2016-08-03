#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

int main( )
{
   int sockfd;
   int len;
   struct sockaddr_in address;
   int result;
   char ch[1024] = "-0.88 -0.5";

   sockfd  = socket(AF_INET, SOCK_STREAM,0);  // criacao do socket

   address.sin_family = AF_INET;
   address.sin_addr.s_addr = inet_addr("127.0.0.1");
   address.sin_port = 3000;
   //strcpy (address.sun_path, "server_socket");
   len = sizeof(address);

   result = connect(sockfd, (struct sockaddr *) &address, len);

   if (result == -1)
   {
      perror ("Houve erro no cliente");
      exit(1);
   }
  
  write(sockfd, &ch,1024);
  read(sockfd, &ch,1024);
  printf( "Output = %s\n",ch);
  close(sockfd);
  exit(0);
}