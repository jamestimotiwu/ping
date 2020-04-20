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
#include <errno.h>
#include <fcntl.h>


static unsigned short csum(unsigned short * data, int len);
static int icmp_packet_create();
static int icmp_sock_init();


/*
 *
 * send ICMP echo requests
 * recieve echo reply
 *
 * report loss / rtt time
 */

unsigned short csum(unsigned short * data, int len){
  int i;
  unsigned int sum = 0;
  unsigned short * ptr;
  unsigned short chcksum;
  
  for(i=len, ptr=data; i > 1; i-=2){ //i-=2 for 2*8=16 bits at time
    sum += *ptr; //sum += 16 bit word at ptr
    ptr+=1;//move ptr to next 16 bit word
  }
  //check if we have an extra 8 bit word
  if (i == 1){
    sum += *((unsigned char*) ptr); //cast ptr to 8 bit unsigned char
  }
  //Fold the cary into the first 16 bits
  sum = (sum & 0xffff) + (sum >> 16);
  //Fold the last cary into the sum
  sum += (sum >> 16);
  // ~ compliments and return
  chcksum = ~sum;
  
  return chcksum;
}

int icmp_packet_create(){
	return 0;
}

int icmp_sock_init() {
	int fd;
	const int val=255;

	fd = -1;
	while (fd < 0) {
		fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		perror("sock_fd");
	}
	/* Configure socket blocking and TLL */
	if (setsockopt(fd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0)
		perror("TTL option");
	/* Non-block socket for recvfrom */
	if ( fcntl(fd, F_SETFL, O_NONBLOCK) != 0)
		perror("Request nonblocking");

	return fd;
}

int host_to_addr(const char* hostname, struct sockaddr_in* addr) {
	struct hostent* host;
	host = gethostbyname(hostname);

	if (host == NULL)
		return -1;
	
	/* Populate socket address struct */
	printf("%s\n", host->h_name);		

	memset(addr, 0, sizeof(*addr));
	addr->sin_family = host->h_addrtype;
	addr->sin_port = 0;
	addr->sin_addr.s_addr = *(long*)host->h_addr;
	return 0;
}

int main(int argc, char** argv) {
	int cnt;
	int sock_fd;
	int nbytes;
	int slen;
	long double rrt;

	char* command;
	struct sockaddr_in addr;
	struct icmphdr pckt;
	struct timespec start, end;
	slen = sizeof(addr);

	/* Check arg buf */
	if (argc == 0) 
		return 0;

	/* Read / parse args and dns lookup */
	command = argv[1];
	if (host_to_addr(command, &addr) == -1)
		return 0;

	printf("Pinging (%s) %d bytes of data. \n", inet_ntoa(addr.sin_addr), nbytes);
	/* Open icmp socket fd*/
	sock_fd = icmp_sock_init();
	
	/* Send echo request */
	nbytes = -1;
	cnt = 1;
	while(1) {
		/* Prepare header*/
		bzero(&pckt, sizeof(pckt));
		pckt.type = ICMP_ECHO;
		pckt.un.echo.id = 1;
		pckt.un.echo.sequence = cnt++;
		pckt.checksum = csum((unsigned short*)&pckt, sizeof(pckt));

		/* Use monotonic time for linearity */
		clock_gettime(CLOCK_MONOTONIC, &start);
		nbytes = sendto(sock_fd, 
					&pckt, 
					sizeof(pckt), 
					0,
					(struct sockaddr*)&addr, 
					sizeof(addr));
		if (nbytes == -1)
			perror("sendto");
		//printf("Pinging (%s) %d bytes of data. \n", inet_ntoa(addr.sin_addr), nbytes);
		nbytes = recvfrom(sock_fd,
					&pckt, 
					sizeof(pckt), 
					0, 
					(struct sockaddr*)&addr, 
					&slen);
		if (nbytes == -1) {
			perror("rcv");
		} else {
			clock_gettime(CLOCK_MONOTONIC, &end);
			rrt = (end.tv_sec - start.tv_sec) * 1000.0 + ((double)(end.tv_nsec - start.tv_nsec)) / 100000.0;
			printf("\nRecieved %d bytes from %s in rrt=%Lf ms\n", nbytes, inet_ntoa(addr.sin_addr), rrt);
		}
		sleep(1);
	}
}


