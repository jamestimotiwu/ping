#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include <arpa/inet.h>

/* Debug */
//#include <libexplain/socket.h>
/*
 *
 * send ICMP echo requests
 * recieve echo reply
 *
 * report loss / rtt time
 */

int main(int argc, char** argv) {
	int sock_fd;
	char* command;
	struct sockaddr_in addr;
	struct hostent* host;
	struct icmphdr icmp;

	/* Check if arg exists */
	if (argc == 0) 
		return 0;

	/* Read / parse args and perform dns lookup */
	command = argv[1];
	host = gethostbyname(command);

	if (host == NULL)
		return 0;
	
	/* Populate socket address struct */
	printf("%s\n", host->h_name);		

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = host->h_addrtype;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = *(long*)host->h_addr;

	printf("%s\n", inet_ntoa(*(struct in_addr*)host->h_addr));

	sock_fd = -1;
	/* Open icmp socket fd*/
	while(sock_fd < 0) {
		sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		printf("%d\n", sock_fd);
	}
	//	fprintf(stderr, "%s\n", explain_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP));
	printf("%d\n", sock_fd);
	/* Prepare icmp */
	int cnt;
	cnt = 0;
	icmp.type = ICMP_ECHO;
	icmp.un.echo.sequence = cnt++;

	/* Send echo request */
	int rcv;
	unsigned char buf[256]; //data buffer

	memcpy(buf, &icmp, sizeof(icmp));
	while(rcv = sendto(sock_fd, buf, sizeof(icmp), 0,
				(struct sockaddr*)&addr, sizeof(addr)) > -1);
	printf("Val: %d\n", rcv);
	/* Poll for packet */
	int slen;
	slen = 0;
	while (1) {
		rcv = recvfrom(sock_fd, buf, sizeof(buf), 0, NULL, &slen);

		//printf("%d", rcv);
	}

	while (1);
}
