#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "wrong argument !");
	}
    
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	int s;
	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if(s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
       	        exit(1);
	}
	connect(sock_fd, result -> ai_addr, result->ai_addrlen);

    // Now that a connection has ben established, we can start sending data to the server.
	while(1)
	{
		char buffer[100];
		fgets(buffer, 50, stdin);		
		write(sock_fd, buffer, strlen(buffer));
		if(buffer[0] == '!')
		{
			puts("Ending client.");
			return 0;
		}
	}
	close(sock_fd);

    return 0;
}
