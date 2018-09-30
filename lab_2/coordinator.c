// Coordinator process for centralized mutual exclusion
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
 
#define PORT 8000
#define MAXLINE 1024
#define RESOURCES 2
#define MAX_QUEUE_SIZE 10
#define REQUEST_TIMEOUT 20

struct sockaddr_in cs_queue[RESOURCES][MAX_QUEUE_SIZE], resource_holder[RESOURCES];
// time_t resource_holder_timeout_check[RESOURCES];
int queue_front[RESOURCES]={0}, queue_back[RESOURCES]={0};

void enqueue(struct sockaddr_in client, int resource_id)
{
	if(queue_back[resource_id] == MAX_QUEUE_SIZE)
	{
		printf("Queue is full\n");
		return;
	}
	cs_queue[resource_id][queue_back[resource_id]] = client;
	queue_back[resource_id]++;
}

struct sockaddr_in dequeue(int resource_id)
{
	struct sockaddr_in top;
	if(queue_back == queue_front)
	{
		printf("Queue is empty");
		return top;
	}
	top = cs_queue[resource_id][queue_front[resource_id]];
	queue_front[resource_id]++;
	return top;
}

int queue_empty(int resource_id)
{
	if(queue_back[resource_id] == queue_front[resource_id])
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int create_connection()
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
 
int main() {
	
    char buffer[MAXLINE];
	int CS_MUTEX[RESOURCES] = {0};
	printf("Initialising the server at port 8000.\n");
    int sockfd = create_connection();
	struct sockaddr_in cliaddr;
	int len, n;
	while(1)
	{
		memset(&cliaddr, 0, sizeof(cliaddr));
		n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
		buffer[n] = '\0';
		char response[MAXLINE];
		printf("%d : %s \n", cliaddr.sin_port, buffer);
		if(!strcmp(buffer, "PING"))
		{
			strcpy(response, "PONG");
		}
		else if(!strncmp(buffer, "REQUEST", 7))
		{
			int resource_id = buffer[8] - '0';
			if(CS_MUTEX[resource_id] == 0)
			{
				CS_MUTEX[resource_id] = 1;
				strcpy(response, "GRANTED");
				resource_holder[resource_id] = cliaddr;
			}
			else
			{
				enqueue(cliaddr, resource_id);
				strcpy(response, "DENIED");
			}
		}
		else if(!strncmp(buffer, "RELEASE", 7))
		{
			int resource_id = buffer[8] - '0';
			if(queue_empty(resource_id))
			{
				CS_MUTEX[resource_id] = 0;
			}
			else{
				struct sockaddr_in queue_top = dequeue(resource_id);
				strcpy(response, "GRANTED");
				sendto(sockfd, (const char *)response, strlen(response), MSG_CONFIRM, (const struct sockaddr *) &queue_top, len);
				resource_holder[resource_id] = queue_top;
			}
			strcpy(response, "ACK");
		}
		else
		{
			strcpy(response, "ERROR");
		}
		sendto(sockfd, (const char *)response, strlen(response), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
	}
	close(sockfd);
	return 0;
}