// Mutex Server
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
#include <pthread.h>

#define MAXLINE 1024
#define MAX_PROCESS 5
#define MAX_QUEUE_SIZE 10000

int sockfd;
int resourceValue=0, resourceLock=0;

pthread_mutex_t LOCK=PTHREAD_MUTEX_INITIALIZER;

struct Message
{
	char state; // L -> Lock resource, R -> Release resource
    int currentValue; // consistent value of resource
    int granted;
};

int cs_queue[MAX_QUEUE_SIZE];
int queue_front=0, queue_back=0;

void enqueue(int client)
{
	if(queue_back == MAX_QUEUE_SIZE)
	{
		printf("Queue is full\n");
		return;
	}
	cs_queue[queue_back] = client;
	queue_back++;
}

int dequeue()
{
	int top;
	if(queue_back == queue_front)
	{
		printf("Queue is empty");
		return top;
	}
	top = cs_queue[queue_front];
	queue_front++;
	return top;
}

int queue_empty()
{
	if(queue_back == queue_front)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

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

void *grantLockThread(void *vargp)
{
    while(1)
    {
        if(queue_empty())
        {
            //sleep(10);
            continue;
        }
        else if(resourceLock == 0)
        {
            pthread_mutex_lock(&LOCK);
            int grant_to = dequeue();
            struct Message newMsg;
            newMsg.state = 'L';
            newMsg.currentValue = resourceValue;
            newMsg.granted = 1;
            send_message(grant_to, sockfd, newMsg);
            resourceLock = 1;
            pthread_mutex_unlock(&LOCK);
        }
    }
}

int main(int argc, char* argv[]) {

	int MY_PORT= atoi(argv[1]);

    struct Message * temp = malloc(sizeof(struct Message));

	printf("Initialising the mutex server at port %d.\n", MY_PORT);
    sockfd = create_connection(MY_PORT);

    pthread_t thread_id; 
    
    pthread_create(&thread_id, NULL, grantLockThread, NULL); 
    // pthread_join(thread_id, NULL); 

    struct sockaddr_in recv_client_addr, send_client_addr;
    int len=sizeof(struct sockaddr_in), n;

    while(1)
    {
        n = recvfrom(sockfd, temp, sizeof(*temp), MSG_WAITALL, ( struct sockaddr *) &recv_client_addr, &len);
        if(temp->state == 'L')
        {
            enqueue(htons(recv_client_addr.sin_port));
        }
        else if(temp->state == 'R')
        {
            pthread_mutex_lock(&LOCK);
            resourceValue = temp->currentValue;
            resourceLock = 0;
            pthread_mutex_unlock(&LOCK);
        }
    }
}
