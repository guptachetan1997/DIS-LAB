// Coordinator for 2 Phase Commit
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
	char type; // V -> Vote Request, c -> Commit OK, a -> Commit Abort, C -> Global commit, A -> Global abort
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

void _2pc(int sockfd, int OTHER_PROCESS_PORTS[], int NUM_PROCESSES, int MY_PORT)
{
    printf("Starting the Voting Phase \n");
    struct sockaddr_in recv_client_addr[NUM_PROCESSES], send_client_addr;
    struct Message * temp = malloc(sizeof(struct Message));
    int len=sizeof(struct sockaddr_in), n, i, flag=1;
    struct Message newMsg;
    
    newMsg.type = 'V';
    
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
		send_message(OTHER_PROCESS_PORTS[i], sockfd, newMsg);
	}
	int commit_flag = 1;
    for(i=0 ; i<NUM_PROCESSES ; i++)
	{
        n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr[i], &len);
        if(temp->type == 'c')
        {
        	commit_flag = commit_flag && 1;
        }
        else if(temp->type == 'a')
        {
			commit_flag = commit_flag && 0;
        }
	}
    printf("Starting the Decision Phase \n");
	char final_decision = (commit_flag ? 'C' : 'A');
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
        newMsg.type = final_decision;
		send_message(OTHER_PROCESS_PORTS[i], sockfd, newMsg);
	}
//	return flag;
}

int main(int argc, char* argv[]) {

	int MY_PORT= atoi(argv[1]);
	int NUM_PROCESSES= atoi(argv[2]);
	int OTHER_PROCESS_PORTS[MAX_PROCESS];
	int i;
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
		OTHER_PROCESS_PORTS[i] = atoi(argv[3+i]);
	}

    struct Message * temp = malloc(sizeof(struct Message));

	printf("Initialising the time server at port %d.\n", MY_PORT);
    int sockfd = create_connection(MY_PORT);

    struct sockaddr_in recv_client_addr, send_client_addr;
    _2pc(sockfd, OTHER_PROCESS_PORTS, NUM_PROCESSES, MY_PORT);
}
