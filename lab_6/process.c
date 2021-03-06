// Lamport Logical clock
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <math.h>

#define MAXLINE 1024
#define MAX_PROCESS 10


int max(int a , int b)
{
    if(a > b)
        return a;
    else
        return b;
}

struct Message
{
	int sender_port;
    int timestamp;
    char msg[MAXLINE];
};

int create_connection(int PORT)
{
    int sockfd;
	struct sockaddr_in servaddr;
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	int optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family    = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
    return sockfd;
}

int main(int argc, char* argv[]) {

	int local_timestamp = rand()%5;
	int increment_time_by = rand()%9+1;

	int MY_PORT= atoi(argv[1]);
	int NEXT_PORT = atoi(argv[2]);
	int INITIATOR = atoi(argv[3]);

    struct Message * temp = malloc(sizeof(struct Message));

	printf("Initialising the server at port %d.\n", MY_PORT);
    int sockfd = create_connection(MY_PORT);

    struct sockaddr_in recv_client_addr, send_client_addr;
    int len, n;
    
    send_client_addr.sin_family = AF_INET;
    send_client_addr.sin_addr.s_addr = INADDR_ANY;
    send_client_addr.sin_port = htons(NEXT_PORT);
    
    if(INITIATOR)
    {
	    struct Message newMsg;
    	newMsg.timestamp = local_timestamp;
    	newMsg.sender_port = MY_PORT;
	    strcpy(newMsg.msg, "MSG");
    	sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *)&send_client_addr, sizeof(send_client_addr));
    }
    int t=10;
   	while(t--)
    {
        struct Message newMsg;
		n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr, &len);
		local_timestamp = max(local_timestamp, temp->timestamp+increment_time_by);
		printf("local time [%d] message from %d\n", local_timestamp, temp->sender_port);
		
		local_timestamp = local_timestamp+increment_time_by;
	    newMsg.timestamp = local_timestamp;
       	newMsg.sender_port = MY_PORT;
	    strcpy(newMsg.msg, "MSG");
    	sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *)
         	&send_client_addr, sizeof(send_client_addr));
	}
}
