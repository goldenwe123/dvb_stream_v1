#include"gn_rtp.h"

int outfd = -1;
struct addrinfo *rtp_outaddrs = NULL;
int ssrc;
uint16_t rtpseq = 0;
int  rtp_base = 12;
char rtp_buf[12 + 188]; //12 header 188 data
char rtp_buffer[RTP_BUFFER_NUMS][12 + 188];

int rtp_head;
int rtp_tail;

char rtp_audio_buffer[RTP_BUFFER_NUMS][12 + 188];

int rtp_audio_head;
int rtp_audio_tail;

void gn_rtp_connect_init(char *outhost ,char *outport){
	
	rtp_head = 0;
    rtp_tail = 0;
	rtp_audio_head =0;
    rtp_audio_tail =0;
	
	// resolve host/port
	if ((outhost != NULL) && (outport != NULL)) {
		int res;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		if ((res = getaddrinfo(outhost, outport, &hints, &rtp_outaddrs)) != 0) {
			fprintf(stderr, "Unable to resolve requested address: %s\n", gai_strerror(res));
			exit(1);
		}
		
		// open output socket
		outfd = socket(rtp_outaddrs->ai_family, rtp_outaddrs->ai_socktype, rtp_outaddrs->ai_protocol);
		if (outfd < 0) {
			fprintf(stderr, "Failed to open output socket\n");
			exit(1);
		}

	}
	srandom(time(NULL));
	
	rtpseq = random();
	ssrc = random();
	
	for(int i=0 ; i< RTP_BUFFER_NUMS; i++) {
		
		rtp_buffer[i][0x0] = 0x80;
		rtp_buffer[i][0x1] = 0x21;
		rtp_buffer[i][0x4] = 0x00; // }
		rtp_buffer[i][0x5] = 0x00; // } FIXME: should really be a valid stamp
		rtp_buffer[i][0x6] = 0x00; // }
		rtp_buffer[i][0x7] = 0x00; // }
		rtp_buffer[i][0x8] = ssrc >> 24;
		rtp_buffer[i][0x9] = ssrc >> 16;
		rtp_buffer[i][0xa] = ssrc >> 8;
		rtp_buffer[i][0xb] = ssrc;

		rtp_audio_buffer[i][0x0] = 0x80;
		rtp_audio_buffer[i][0x1] = 0x21;
		rtp_audio_buffer[i][0x4] = 0x00; // }
		rtp_audio_buffer[i][0x5] = 0x00; // } FIXME: should really be a valid stamp
		rtp_audio_buffer[i][0x6] = 0x00; // }
		rtp_audio_buffer[i][0x7] = 0x00; // }
		rtp_audio_buffer[i][0x8] = ssrc >> 24;
		rtp_audio_buffer[i][0x9] = ssrc >> 16;
		rtp_audio_buffer[i][0xa] = ssrc >> 8;
		rtp_audio_buffer[i][0xb] = ssrc;	
	}	
	
	
}

void gn_rtp_put(char* data)
{
	
	//printf("RTP video buffer %d %d \n",rtp_head,rtp_tail);
	if( rtp_head == ( (rtp_tail+1) % RTP_BUFFER_NUMS ) ){
		
		while(1) {
			printf("****************RTP video buffer full***********\n");
		}
		return ;
	}
	

	memcpy(rtp_buffer[rtp_tail]+rtp_base , data , 188);
	
	rtp_tail++;
	rtp_tail %= RTP_BUFFER_NUMS;
	
	return;
		
	
}	

void gn_rtp_send()
{
	
  	if(  rtp_head == rtp_tail ) {
		//printf("RTP no data\n");
		return ;
	}	
			
	rtp_buffer[rtp_head][2] = rtpseq >> 8;
    rtp_buffer[rtp_head][3] = rtpseq;		
	rtpseq++;
  
  if (sendto(outfd, rtp_buffer[rtp_head], rtp_base + 188, 0, rtp_outaddrs->ai_addr, rtp_outaddrs->ai_addrlen) < 0) {
		if (errno != EINTR) {
			fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!Socket send failure: %m !!!!!!!!!!!!!!\n");
				
		}
		
  }
  
  rtp_head++;
  rtp_head %= RTP_BUFFER_NUMS;

  usleep(100);

}


void gn_rtp_audio_put(char* data)
{
	if( rtp_audio_head == ( (rtp_audio_tail+1) % RTP_BUFFER_NUMS ) ){
		printf("RTP audio buffer full %d %d \n",rtp_audio_head,rtp_audio_tail);
		return ;
	}
	
	memcpy(rtp_audio_buffer[rtp_audio_tail]+rtp_base , data , 188);
	
	rtp_audio_tail++;
	rtp_audio_tail %= RTP_BUFFER_NUMS;
	
	return;
		
	
}	

void gn_rtp_audio_send()
{
	
  	if(  rtp_audio_head == rtp_audio_tail ) {
		//printf("RTP no data\n");
		return ;
	}	
			
	rtp_audio_buffer[rtp_audio_head][2] = rtpseq >> 8;
    rtp_audio_buffer[rtp_audio_head][3] = rtpseq;		
	rtpseq++;
  
  if (sendto(outfd, rtp_audio_buffer[rtp_audio_head], rtp_base + 188, 0, rtp_outaddrs->ai_addr, rtp_outaddrs->ai_addrlen) < 0) {
		if (errno != EINTR) {
			fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!Socket send failure: %m !!!!!!!!!!!!!!\n");
				
		}
		
  }
  
  rtp_audio_head++;
  rtp_audio_head %= RTP_BUFFER_NUMS;

  usleep(100);

}		