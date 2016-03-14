#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#define MESG_SIZE (256)
void* processClient(void* arg);
void handle_sigint(int signal)
{
	puts("Ending Server");
	exit(0);
}
int main(int argc, char** argv)
{
	signal(SIGINT, handle_sigint);
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	int optval = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));	
	
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int s;
	s = getaddrinfo(NULL, argv[1], &hints, &result);
	if(s != 0)
	{
		fprintf(stderr, "getaddrinfo:  %s\n", gai_strerror(s));
		exit(1);
	}

	if(bind(sock_fd, result -> ai_addr, result -> ai_addrlen) != 0)
	{
		perror("bind()");
		exit(1);
	}
	if(listen(sock_fd, 10) != 0)
	{
		exit(1);
		perror("listen()");
	}
	struct sockaddr_in *result_addr = (struct sockaddr_in *) result -> ai_addr;
	printf("Listening on file descriptor %d, port %d\n", sock_fd, ntohs(result_addr->sin_port));
	
	printf("Waiting for connection...\n");
	pthread_t thread_id;
	int client_fd;
	while(client_fd = accept(sock_fd, NULL, NULL))
	{
		printf("Connection made: client_fd = %d\n", client_fd);	
		/*if(pthread_create(&thread_id, NULL, processClient, (void *)&client_fd) < 0 )
		{
			perror("could not create thread");
			exit(1);		
		}*/
		char buffer[100];
		int len = read(client_fd, buffer, sizeof(buffer)-1);
		while(len != 0)
		{
			buffer[len] = '\0';
	       		printf("%s", buffer);
			if(buffer[0] == '!')
			{
				puts("Ending server.");
				return 0;
			}
			len = read(client_fd, buffer, sizeof(buffer)-1);
		}
	}
	
	if(client_fd < 0)
	{
		perror("accpet failed");
		return 1;
	}
	//pthread_join(thread_id, NULL);
	close(client_fd);
	close(sock_fd);
	
	return 0; 
 

}

/*void* processClient(void* arg) {
	int client_fd = (intptr_t) arg;
	//pthread_detach(pthread_self()); // no join() required
    	int len;
	char *buffer = calloc(1000,1);
	char *message = calloc(1000,1);
	buffer = "Hi! I'm your server!";
	write(client_fd, buffer, sizeof(buffer));
	buffer = "Now type in something and I should repeat what you typed in.";
	write(client_fd, buffer, sizeof(buffer));

	while(len = recv(client_fd, message, 1000, 0) > 0)
	{
		message[len] = '\0';
		write(client_fd, message, strlen(message));
		memset(message, 0, 1000);
	}
	
     
       if(len == 0)
       {
          puts("Client disconnected");
          fflush(stdout);
       }
       else if(len == -1)
       {
          perror("recv failed");
       }
         
       return NULL;
    // Continue reading input from the client and print to stdout
    
  
}
