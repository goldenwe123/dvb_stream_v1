#include "gn_tcp.h"

#define SERV_PORT 80
#define MAXNAME 1024

 int socket_fd;      /* file description into transport */
 int recfd;     /* file descriptor to accept        */
 int length;     /* length of address structure      */
 int nbytes;     /* the number of read **/
 struct sockaddr_in myaddr; /* address of this service */
 struct sockaddr_in client_addr; /* address of client    */
 
char tcp_buffer[TCP_BUFFER_NUMS][188];
char tcp_audio_buffer[TCP_BUFFER_NUMS][188];

int  tcp_head;
int  tcp_tail;

int  tcp_audio_head;
int  tcp_audio_tail;

uint64_t video_pts;
uint64_t audio_pts;

uint64_t tcp_get_pts(char* pts)
{
	uint64_t pts_0 = (pts[0] >> 1) & 0x07;
	uint64_t pts_1 = pts[1];
	uint64_t pts_2 = pts[2] >> 1;
	uint64_t pts_3 = pts[3];
	uint64_t pts_4 = pts[4] >> 1;
	
	return (pts_0 << 30) + (pts_1 << 22) + (pts_2 << 15) + (pts_3 << 7) + (pts_4 );
	
}

uint64_t gn_tcp_get_video_pts()
{
		return video_pts;
}

uint64_t gn_tcp_get_audio_pts()
{
		return audio_pts;
}

		

int isVideo()
{
	
	if(tcp_buffer[tcp_head][4] == 0x00 && tcp_buffer[tcp_head][5] == 0x00 && tcp_buffer[tcp_head][6] == 0x01 && tcp_buffer[tcp_head][7] == 0xE0) {
		
		return 1;
	}

		return 0;
	
}

int isAudio()
{
	
	if(tcp_audio_buffer[tcp_audio_head][4] == 0x00 && tcp_audio_buffer[tcp_audio_head][5] == 0x00 && tcp_audio_buffer[tcp_audio_head][6] == 0x01 && tcp_audio_buffer[tcp_audio_head][7] == 0xC0) {
		
		return 1;
	}

		return 0;
	
}

int isVideoEmpty()
{
	
		
	if(  tcp_head == tcp_tail ) {
		//printf("RTP no data\n");
		return 1;
	}	

		return 0;
	
}

int isAudioEmpty()
{
	
		
	if(  tcp_audio_head == tcp_audio_tail ) {
		//printf("RTP no data\n");
		return 1;
	}	

		return 0;
	
}			
			


void gn_tcp_init()
{
	
	tcp_head = 0;
	tcp_tail = 0;
	tcp_audio_head =0;
    tcp_audio_tail =0;
	video_pts = 0;
	audio_pts = 0;
	
/*                             
 *      Get a socket into TCP/IP
 */
 if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) <0) {
  perror ("socket failed");
  exit(1);
 }
 
 /*
 *    Set up our address
 */
 bzero ((char *)&myaddr, sizeof(myaddr));
 myaddr.sin_family = AF_INET;
 myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 myaddr.sin_port = htons(SERV_PORT);
 
 /*
 *     Bind to the address to which the service will be offered
 */
 if (bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) <0) {
  perror ("bind failed");
  exit(1);
 }

}

void gn_tcp_listener()
{
	
	if (listen(socket_fd, 20) <0) {
	  perror ("listen failed");
	  exit(1);
	}
	length = sizeof(client_addr);
    printf("Server is ready to receive !!\n");
    printf("Can strike Cntrl-c to stop Server >>\n");
	
	if ((recfd = accept(socket_fd,
      (struct sockaddr_in *)&client_addr, &length)) <0) {
		perror ("could not accept call");
		exit(1);
    }	
}

void gn_tcp_put(char* data)
{
	
	//printf("RTP video buffer %d %d \n",rtp_head,rtp_tail);
	if( tcp_head == ( (tcp_tail+1) % TCP_BUFFER_NUMS ) ){
		
		while(1) {
			printf("****************TCP video buffer full*********** %d %d \n",tcp_head,tcp_tail);
		}
		return ;
	}
	

	memcpy(tcp_buffer[tcp_tail] , data , 188);
	
	tcp_tail++;
	tcp_tail %= TCP_BUFFER_NUMS;
	//printf("TCP video buffer  %d %d \n",tcp_head,tcp_tail);
	return;
		
}

/*
ssize_t  gn_tcp_send(void *buf)
{
	

	
	length = send(recfd,buf, 188, 0  );
	if(length == -1) {
	   perror ("write to client error");
	   exit(1);
	}
	return length;
	
}		
*/
ssize_t  gn_tcp_send()
{
	
	if(  tcp_head == tcp_tail ) {
		//printf("RTP no data\n");
		return ;
	}	
	
	if(tcp_buffer[tcp_head][4] == 0x00 && tcp_buffer[tcp_head][5] == 0x00 && tcp_buffer[tcp_head][6] == 0x01 && tcp_buffer[tcp_head][7] == 0xE0) {
		
		video_pts = tcp_get_pts(&tcp_buffer[tcp_head][0x0d]);
		//printf("VIDEO PTS: %d AUDIO PTS: %d\n",video_pts,audio_pts);
	}	
	
	length = send(recfd, tcp_buffer[tcp_head], 188, 0  );
	if(length == -1) {
	   perror ("write to client error");
	   exit(1);
	}
	
	
	tcp_head++;
    tcp_head %= TCP_BUFFER_NUMS;
	
	usleep(100);
	
	return length;
	
}	

void gn_tcp_audio_put(char* data)
{
	
	//printf("RTP video buffer %d %d \n",rtp_head,rtp_tail);
	if( tcp_audio_head == ( (tcp_audio_tail+1) % TCP_BUFFER_NUMS ) ){
		
		while(1) {
			printf("****************TCP audio buffer full*********** %d %d \n",tcp_head,tcp_tail);
		}
		return ;
	}
	

	memcpy(tcp_audio_buffer[tcp_audio_tail] , data , 188);
	
	tcp_audio_tail++;
	tcp_audio_tail %= TCP_BUFFER_NUMS;
	
	return;
		
}

ssize_t  gn_tcp_audio_send()
{
	static uint64_t pts_pre = 0;
	if(  tcp_audio_head == tcp_audio_tail ) {
		//printf("RTP no data\n");
		return ;
	}

	//if( (audio_sec - video_sec) > 0.01 ) {
	  //return 0;	
	//}	
	
	
	if(tcp_audio_buffer[tcp_audio_head][4] == 0x00 && tcp_audio_buffer[tcp_audio_head][5] == 0x00 && tcp_audio_buffer[tcp_audio_head][6] == 0x01 && tcp_audio_buffer[tcp_audio_head][7] == 0xC0) {
		
		audio_pts = tcp_get_pts(&tcp_audio_buffer[tcp_audio_head][0x0d]);
	}	
	
	
	length = send(recfd, tcp_audio_buffer[tcp_audio_head], 188, 0  );
	if(length == -1) {
	   perror ("write to client error");
	   exit(1);
	}
	
	tcp_audio_head++;
    tcp_audio_head %= TCP_BUFFER_NUMS;
	
	usleep(100);
		
	return length;
	
}	

	