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
#include <errno.h>
#include <fcntl.h>
/*
 *
 * send ICMP echo requests
 * recieve echo reply
 *
 * report loss / rtt time
 */

struct header {
	struct icmphdr hdr;
	char msg[64 - sizeof(struct icmphdr)];
};

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
	addr.sin_port = 0;
	addr.sin_addr.s_addr = *(long*)host->h_addr;

	printf("%s\n", inet_ntoa(*(struct in_addr*)host->h_addr));

	sock_fd = -1;
	/* Open icmp socket fd*/
	while(sock_fd < 0) {
		sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		printf("%d\n", sock_fd);
		exit(1);
	}
	printf("%d\n", sock_fd);

	/* Setup socket*/
	const int val = 255;
	if (setsockopt(sock_fd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("TTL option");
	if ( fcntl(sock_fd, F_SETFL, O_NONBLOCK) != 0)
		perror("Request nonblocking i/o");

	/* Prepare header*/
	int cnt = 0;
	struct header buffer;
	buffer.hdr.un.echo.sequence = cnt++;
	buffer.hdr.type = ICMP_ECHO;
	memset(buffer.msg, 0, sizeof(buffer.msg));

	/* Prepare icmp */
	//int cnt;
	//cnt = 0;

	icmp.type = ICMP_ECHO;
	//icmp.un.echo.sequence = cnt++;

	/* Send echo request */
	int rcv;
	unsigned char buf[64]; //data buffer
	bzero(buf, sizeof(buf));
	rcv = -1;
	memcpy(buf, &icmp, sizeof(icmp));
	while (rcv < 0) {
		rcv = sendto(sock_fd, &buffer, sizeof(buffer), 0,
				(struct sockaddr*)&addr, sizeof(addr));
		if (rcv == -1)
			perror("sendto");
	}
	printf("Val: %d\n", rcv);
	/* Poll for packet */
	int slen;
	slen = 0;
	while (1) {
		rcv = recvfrom(sock_fd, buf, sizeof(buf), 0, NULL, &slen);
		if(rcv == -1) {
			perror("Err: ");
		}
		//printf("%d", rcv);
	}

	while (1);
}
