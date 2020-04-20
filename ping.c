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


static unsigned short csum(unsigned short * data, int len);

/*
 *
 * send ICMP echo requests
 * recieve echo reply
 *
 * report loss / rtt time
 */

#define PACKETSIZE	64
struct packet
{
	struct icmphdr hdr;
	char msg[PACKETSIZE-sizeof(struct icmphdr)];
};


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

int main(int argc, char** argv) {
	int sock_fd;
	char* command;
	struct sockaddr_in addr;
	struct hostent* host;
	//unsigned char buf[64];
	//struct icmphdr icmp;

	int i, cnt=1;
	struct packet pckt;
	int slen;
	slen = sizeof(addr);
	/* Check arg buf */
	if (argc == 0) 
		return 0;

	/* Read / parse args and dns lookup */
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

	/* Open icmp socket fd*/
	sock_fd = icmp_sock_init();
	/* Send echo request */
	int rcv;
	rcv = -1;
	unsigned char data_recv[256];
	while(1) {
		/* Prepare header*/
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = 1;
		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = cnt++;
		pckt.hdr.checksum = csum((unsigned short*)&pckt, sizeof(pckt));
			
		rcv = -1;
		rcv = sendto(sock_fd, 
					&pckt, 
					sizeof(pckt), 
					0,
					(struct sockaddr*)&addr, 
					sizeof(addr));
		if (rcv == -1)
			perror("sendto");
		printf("Rcv: %d", rcv);
		rcv = recvfrom(sock_fd, &pckt, sizeof(pckt), 0, (struct sockaddr*)&addr, &slen);
		if (rcv == -1) {
			perror("rcv");
		} else {
			printf("\nrecieved %s\n", inet_ntoa(addr.sin_addr));
		}
		sleep(1);
	}
while (1);
}


