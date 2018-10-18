// Bully algorithm based election

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

#define MAXLINE 1024
#define MAX_PROCESS 10
int ELECTION_FLAG=0;

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

void send_message(int dest_port, int sockfd, char response[MAXLINE])
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family    = AF_INET; // IPv4
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_port = htons(dest_port);
	sendto(sockfd, (const char *)response, strlen(response), MSG_CONFIRM, (const struct sockaddr *) &client_addr, sizeof(client_addr));
}

int send_election_message(int sockfd, int OTHER_PROCESS_PORTS[], int NUM_PROCESSES, int MY_PORT)
{
	int i;
	char response[MAXLINE];
	strcpy(response, "ELEC");
	int flag=1;
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
		if(OTHER_PROCESS_PORTS[i] > MY_PORT)
		{
			send_message(OTHER_PROCESS_PORTS[i], sockfd, response);
			flag=0;
		}
	}
	ELECTION_FLAG = 1;
	return flag;
}

void send_coord_message(int sockfd, int OTHER_PROCESS_PORTS[], int NUM_PROCESSES, int MY_PORT)
{
	int i;
	char response[MAXLINE];
	strcpy(response, "CORD");
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
		if(1)//OTHER_PROCESS_PORTS[i] != MY_PORT)
		{
			send_message(OTHER_PROCESS_PORTS[i], sockfd, response);
		}
	}
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
	int IS_INITIATOR= atoi(argv[3+NUM_PROCESSES]);

    char buffer[MAXLINE];

	printf("Initialising the server at port %d.\n", MY_PORT);
    int sockfd = create_connection(MY_PORT);

    struct sockaddr_in recv_client_addr;
    int len, n;

    char response[MAXLINE];

    if(IS_INITIATOR)
    {
		send_election_message(sockfd, OTHER_PROCESS_PORTS, NUM_PROCESSES, MY_PORT);
	}
    while(1)
    	{
	    	// memset(&recv_client_addr, 0, sizeof(recv_client_addr));
			n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &recv_client_addr, &len);
			buffer[n] = '\0';
			printf("MSG RECVD : %s\n", buffer);
			if(!strcmp(buffer, "ELEC"))
			{
				sleep(MY_PORT%5);
				char response[MAXLINE];
				strcpy(response, "EACK");
				sendto(sockfd, (const char *)response, strlen(response), MSG_CONFIRM, (const struct sockaddr *) &recv_client_addr, sizeof(recv_client_addr));
				sleep(MY_PORT%5);
				if(ELECTION_FLAG == 0){
					if(send_election_message(sockfd, OTHER_PROCESS_PORTS, NUM_PROCESSES, MY_PORT)){
						send_coord_message(sockfd, OTHER_PROCESS_PORTS, NUM_PROCESSES, MY_PORT);
						printf("I AM COORDINATOR.\n");
						}
				}

			}
			else if(!strcmp(buffer, "EACK"))
			{
				//stop_election()
				ELECTION_FLAG = 1;
				continue;
			}
			else if(!strcmp(buffer, "CORD"))
			{
				//set_new_coordinator
				COORDINATOR_PORT = ntohs(recv_client_addr.sin_port);
				//printf("COORDINATOR WAS SET\n");
			}
		}
}
