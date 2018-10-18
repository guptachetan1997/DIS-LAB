// Central time coordinator for berkeley time synch
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
	char type; // P->poll for local time, S->synch time
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

int send_time_poll_message(int sockfd, int OTHER_PROCESS_PORTS[], int NUM_PROCESSES, int MY_PORT)
{
	int i;
	char response[MAXLINE];
	strcpy(response, "ELEC");
	int flag=1;
	for(i=0 ; i<NUM_PROCESSES ; i++)
	{
	    send_client_addr.sin_family = AF_INET;
	    send_client_addr.sin_addr.s_addr = INADDR_ANY;
	    send_client_addr.sin_port   = htons(OTHER_PROCESS_PORTS[i]);
		if(OTHER_PROCESS_PORTS[i] > MY_PORT)
		{
			send_message(OTHER_PROCESS_PORTS[i], sockfd, response);
			flag=0;
		}
	}
	return flag;
}

int send_time_synch_message(int sockfd, int OTHER_PROCESS_PORTS[], int NUM_PROCESSES, int MY_PORT)
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

    if(IS_INITIATOR)
    {
        struct Message newMsg;
        newMsg.point = 0;
        newMsg.COORD = MY_PORT;
        newMsg.type = 'E';
        newMsg.participants[newMsg.point] = MY_PORT;
        sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *)
         &send_client_addr, sizeof(send_client_addr));
        ELECTION_FLAG=1;
	}
    while(1)
    	{
	    	// memset(&recv_client_addr, 0, sizeof(recv_client_addr));
            struct Message newMsg;
			n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr, &len);
            switch(temp->type)
            {
                case 'E':
                    printf("Msg Recvd : ELECTION {");
                    if (ELECTION_FLAG)
                    {
                        printf("}\n");
                        newMsg.type = 'C';
                        newMsg.COORD = temp->COORD;
                        newMsg.point = temp->point;
                        for(int i=0 ; i<=temp->point ; i++)
                            newMsg.participants[i] = temp->participants[i];
                        for(int i=0 ; i<=temp->point ; i++){
                            send_client_addr.sin_family = AF_INET;
                            send_client_addr.sin_addr.s_addr = INADDR_ANY;
                            send_client_addr.sin_port   = htons(temp->participants[i]);
                            sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *)
                             &send_client_addr, sizeof(send_client_addr));
                        }
                    }
                    else
                    {
                        newMsg.point = temp->point+1;
                        newMsg.type = 'E';
                        newMsg.COORD = max(temp->COORD, MY_PORT);
                        for(int i=0 ; i<=temp->point ; i++)
                        {
                            printf("%d,", temp->participants[i]);
                            newMsg.participants[i] = temp->participants[i];
                        }
                        printf("}\n");
                        newMsg.participants[newMsg.point] = MY_PORT;
                        sendto(sockfd, (struct Message*)&newMsg, (1024+sizeof(newMsg)), 0, (struct sockaddr *)
                         &send_client_addr, sizeof(send_client_addr));
                         ELECTION_FLAG=1;
                    }
                    break;
                case 'A':
                    printf("Msg Recvd : Ack\n");
                    break;
                case 'C':
                    printf("Msg Recvd : COORDINATOR\n");
                    printf("New COORDINATOR is : %d\n", temp->COORD);
                    break;
            };
		}
}
