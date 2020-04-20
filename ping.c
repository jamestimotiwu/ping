#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include <arpa/inet.h>

/* Debug */
#include <errno.h>
#include <fcntl.h>


static unsigned short csum(unsigned short* data, int len);
static int icmp_sock_init();
static int host_to_addr(const char* hostname, struct sockaddr_in* addr);

/*
 * send ICMP echo requests
 * recieve echo reply
 * report loss / rtt time
 */
unsigned short csum(unsigned short* data, int len){
	int i;
	unsigned int sum = 0;
	unsigned short *ptr;
	unsigned short checksum;
  
	for(i=len, ptr=data; i > 1; i-=2){ 
		sum += *ptr; 
		ptr+=1;
	}
	if (i == 1){
		sum += *((unsigned char*) ptr); 
	}
	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);
	checksum = ~sum;
  
	return checksum;
}

/*
 * setup icmp socket
 * Input: None
 * Output: fd - file descriptor
 * Side effect: Set up file descriptor
 */
int icmp_sock_init() {
	int fd;
	const int val=255;

	fd = -1;
	while (fd < 0) {
		fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	}
	/* Configure socket blocking and TLL */
	if (setsockopt(fd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("TTL option");
	/* Non-block socket for recvfrom */
	if ( fcntl(fd, F_SETFL, O_NONBLOCK) != 0)
		perror("Request nonblocking");

	return fd;
}

/*
 * setup icmp socket
 * Input: hostname - hostname in string
 *		  addr - pointer to address struct
 * Output: 0 if success, -1 if fail
 * Side effect: modify addr structure given hostname
 */
int host_to_addr(const char* hostname, struct sockaddr_in* addr) {
	struct hostent* host;
	host = gethostbyname(hostname);

	if (host == NULL)
		return -1;
	
	/* Populate socket address struct */
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = host->h_addrtype;
	addr->sin_port = 0;
	addr->sin_addr.s_addr = *(long*)host->h_addr;
	return 0;
}

int main(int argc, char** argv) {
	int cnt;
	int recieved;
	int sock_fd;
	int nbytes;
	int slen;
	double rrt;
	double loss;

	char* command;
	struct sockaddr_in addr;
	struct icmphdr pckt;
	struct timespec start, end;
	slen = sizeof(addr);

	/* Check arg buf */
	if (argc == 1) { 
		printf("No arguments given. Usage: ping <hostname or ip>\n");
		return 0;
	}
	/* Read / parse args and dns lookup */
	command = argv[1];
	if (host_to_addr(command, &addr) == -1)
		return 0;

	printf("Pinging (%s) %ld bytes of data. \n", inet_ntoa(addr.sin_addr), sizeof(pckt));
	/* Open icmp socket fd*/
	sock_fd = icmp_sock_init();
	
	/* Send echo request */
	nbytes = -1;
	cnt = 1;
	recieved = 1;
	while(1) {
		/* Prepare header*/
		memset(&pckt, 0, sizeof(pckt));
		pckt.type = ICMP_ECHO;
		pckt.un.echo.id = 1;
		pckt.un.echo.sequence = cnt++;
		pckt.checksum = csum((unsigned short*)&pckt, sizeof(pckt));

		/* Use monotonic time for linearity */
		clock_gettime(CLOCK_MONOTONIC, &start);
		nbytes = sendto(sock_fd, &pckt, sizeof(pckt), 0,(struct sockaddr*)&addr, sizeof(addr));
		if (nbytes == -1)
			perror("sendto");
		nbytes = recvfrom(sock_fd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&addr, &slen);
		if (nbytes >= 0) {
			clock_gettime(CLOCK_MONOTONIC, &end);
			rrt = (end.tv_sec - start.tv_sec) * 1000.0 + ((double)(end.tv_nsec - start.tv_nsec)) / 100000.0;
			recieved++;
			loss = ((double)(cnt - recieved)/(double)cnt);
			printf("%d bytes from %s: rrt=%f ms, loss=%f\n", nbytes, inet_ntoa(addr.sin_addr), rrt, loss);
		} else {
			recieved++;
			if (cnt <= 1)
				printf("Packet dropped.\n");
		}

		//delay
		sleep(1);
	}
}


