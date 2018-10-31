// Process which acquires mutex and updates a replicated data store

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

int MY_PORT, MUTEX_SERVER_PORT,IS_OWNER, RESOURCE_VALUE=0;

int max(int a , int b)
{
    if(a > b)
        return a;
    else
        return b;
}

struct Message
{
	char state; // L -> Lock resource, R -> Release resource
    int currentValue; // consistent value of resource
    int granted;
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

void acquire_lock(int sockfd)
{
    if(IS_OWNER == 1)
        return;
    struct Message newMsg, * temp = malloc(sizeof(struct Message));
    newMsg.state = 'L';
    send_message(MUTEX_SERVER_PORT, sockfd, newMsg);
    struct sockaddr_in recv_client_addr, send_client_addr;
    int len=sizeof(struct sockaddr_in), n;
    n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr, &len);
    // insert some logic
    RESOURCE_VALUE = temp->currentValue;
    IS_OWNER = 1;
}

void release_lock(int sockfd)
{
    if(IS_OWNER == 0)
        return;
    struct Message newMsg;
    newMsg.state = 'R';
    newMsg.currentValue = RESOURCE_VALUE;
    send_message(MUTEX_SERVER_PORT, sockfd, newMsg);
    IS_OWNER = 0;
}

int main(int argc, char* argv[]) {
    srand ( time(NULL) );
	MY_PORT= atoi(argv[1]);
	MUTEX_SERVER_PORT= atoi(argv[2]);
	printf("Initialising server at port %d\n", MY_PORT);
    sleep(10 - MY_PORT % 10);
    int sockfd = create_connection(MY_PORT);

    struct sockaddr_in recv_client_addr, send_client_addr;
    int len=sizeof(struct sockaddr_in), n;
    int counter = 10;
    while(counter){
        counter--;
        double random_number = (double)rand() / (double)((unsigned)RAND_MAX + 1);
        if(random_number >= 0.7)
        {
            random_number = (double)rand() / (double)((unsigned)RAND_MAX + 1);
            if(random_number >= 0.6)
            {
                printf("[Requesting Edit] ");
                acquire_lock(sockfd);
                printf("[GRANTED]\n");
                RESOURCE_VALUE += 1;
                release_lock(sockfd);
            }
            else
            {
                printf("[Requesting Read] ");
                acquire_lock(sockfd);
                printf("[GRANTED] -> ");
                printf("Resource Value : %d\n", RESOURCE_VALUE);
                release_lock(sockfd);
            }
        }
        else
        {
            continue;
        }
    }
    sleep(10);
    printf("---------------FINAL CONSISTENT VALUE---------------\n");
    printf("[Requesting Read] ");
    acquire_lock(sockfd);
    printf("[GRANTED] -> ");
    printf("Resource Value : %d\n", RESOURCE_VALUE);
    release_lock(sockfd);
}
