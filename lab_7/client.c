// Central time coordinator for Berkeley time synch
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
#include <sys/poll.h>

#define MAXLINE 1024
#define MAX_PROCESS 5


int max(int a , int b)
{
    if(a > b)
        return a;
    else
        return b;
}

struct Message
{
	char type; // P->poll for local time, S->synch time, R -> reply for time poll
	int timestamp;
	int change;
	
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

void send_message(int dest_port, int sockfd, struct Message newMsg)
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family    = AF_INET; // IPv4
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_port = htons(dest_port);
    sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
}

int main(int argc, char* argv[]) {
    srand ( time(NULL) );
    int local_time = 100 - rand()%20;
	int MY_PORT= atoi(argv[1]);
	int TIME_SERVER_PORT= atoi(argv[2]);

    struct Message * temp = malloc(sizeof(struct Message));

	printf("Initialising the time server at port %d [local time : %d].\n", MY_PORT, local_time);
    int sockfd = create_connection(MY_PORT);

    struct sockaddr_in recv_client_addr, send_client_addr;
    int len, n;

    while(1){
        n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr, &len);
        struct Message newMsg;
        if(temp->type == 'P')
        {
            printf("Server sent a time poll message.\n");
            newMsg.type = 'R';
            newMsg.timestamp = local_time;
            newMsg.change = 0;
            send_message(TIME_SERVER_PORT, sockfd, newMsg);
        }
        else if(temp->type == 'S')
        {
            printf("New Synch time recieved from server : %d\n", temp->timestamp);
            if(temp->change < 0)
                printf("Set clock ahead by %d\n", -temp->change);
            else
                printf("Set clock slow by %d\n", temp->change);
        }
    }
}
