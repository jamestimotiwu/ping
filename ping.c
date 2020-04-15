#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char** argv) {
	int socket_fd;
	char* command;
	struct sockaddr_in addr;
	struct hostent* host;

	/* Check if arg exists */
	if (argc == 0) 
		return 0;

	/* Read / parse args and perform dns lookup */
	command = argv[1];
	host = gethostbyname(command);

	if (host == NULL)
		return 0;
	
	/* Populate socket address struct */
	//addr->sin_family = host->h_addrtype;
	//addr->sin_port = ;
	//addr->sin_addr.s_addr = *(host->h_addr);

	printf("%s", host->h_name);
	

}
