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

void send_message(int dest_port, int sockfd, struct Message newMsg, struct sockaddr_in client)
{
    struct sockaddr_in client_addr;
    if(dest_port != -1)
    {
        memset(&client_addr, 0, sizeof(client_addr));
	    client_addr.sin_family    = AF_INET; // IPv4
	    client_addr.sin_addr.s_addr = INADDR_ANY;
	    client_addr.sin_port = htons(dest_port);
    }
    else
    {
        client_addr = client;
    }
    sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
}

int send_time_poll_message(int sockfd, int OTHER_PROCESS_PORTS[], int NUM_PROCESSES, int MY_PORT)
{
    printf("Starting the Berkeley Time Snchronization \n");
    struct sockaddr_in recv_client_addr[NUM_PROCESSES], send_client_addr;
    struct Message * temp = malloc(sizeof(struct Message));
    int len=sizeof(struct sockaddr_in), n, i, flag=1;
    struct Message newMsg;
    newMsg.type = 'P';
    newMsg.timestamp = -1;
    newMsg.change = 0;
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
		send_message(OTHER_PROCESS_PORTS[i], sockfd, newMsg, send_client_addr);
	}
    int total_time_sum=0, local_client_times[NUM_PROCESSES];
    for(i=0 ; i<NUM_PROCESSES ; i++)
	{
        n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr[i], &len);
        if(temp->type == 'R')
        {

            local_client_times[i] = temp->timestamp;
            total_time_sum += temp->timestamp;
            printf("Local time of process %d : %d\n", ntohs(recv_client_addr[i].sin_port), temp->timestamp);
        }
        else
        {
            printf("Error occured\n");
            exit(0);
        }
	}
    printf("New Synch Time : %d \n", total_time_sum/NUM_PROCESSES);
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
        newMsg.type = 'S';
        newMsg.timestamp = total_time_sum/NUM_PROCESSES;
        newMsg.change = local_client_times[i] - (total_time_sum/NUM_PROCESSES);
		send_message(-1, sockfd, newMsg, recv_client_addr[i]);
	}
	return flag;
}

int main(int argc, char* argv[]) {

	int MY_PORT= atoi(argv[1]);
	int NUM_PROCESSES= atoi(argv[2]);
	int OTHER_PROCESS_PORTS[MAX_PROCESS];
	int COORDINATOR_PORT;
	int i;
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
		OTHER_PROCESS_PORTS[i] = atoi(argv[3+i]);
	}

    struct Message * temp = malloc(sizeof(struct Message));

	printf("Initialising the time server at port %d.\n", MY_PORT);
    int sockfd = create_connection(MY_PORT);

    struct sockaddr_in recv_client_addr, send_client_addr;
    int len, n;

    char response[MAXLINE];

    send_time_poll_message(sockfd, OTHER_PROCESS_PORTS, NUM_PROCESSES, MY_PORT);
}
