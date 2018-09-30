// Client service for mutual exclusion
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
 
#define PORT 8000
#define MAXLINE 1024
 
// Driver code
int main(int argc, char* argv[]) {
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr;
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    int optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    int n, len;
    char *hello = "PING", *request, *release="RELEASE";
    request = malloc(strlen(argv[1]) + 8);
    strcpy(request, "REQUEST_");
    strcat(request, argv[1]);
    sendto(sockfd, (const char *)hello, strlen(hello), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
    buffer[n] = '\0';
    if(!strcmp(buffer, "PONG"))
    {
        sendto(sockfd, (const char *)request, strlen(request), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
        buffer[n] = '\0';
        if(!strcmp(buffer, "DENIED"))
        {
            printf("Request denied\n");
            n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
            buffer[n] = '\0';
        }
        printf("Performing critical section\n");
        sleep(10);
        sendto(sockfd, (const char *)release, strlen(release), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    }
    close(sockfd);
    return 0;
}