#include"gn_dvb.h"
#include "mpi_decoder.h"
#include "mpi_encoder.h"
#include "RgaApi.h"
#include <pthread.h>

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 300
#define	FPS	30

#define	FRAME_WIDTH	1280
#define	FRAME_HEIGHT 720

MppFrame buffer[BUFFER_SIZE];
RK_U32 buffer_flag[BUFFER_SIZE]={0};

DecoderData *decoder = NULL;
EncoderData *encoder = NULL;

struct gn_program *program;
struct gn_dvb *device;
int channel = 11;

static int outfd = -1;
static struct addrinfo *outaddrs = NULL;

int ssrc;
static uint16_t rtpseq = 0;

void connect_init(char *outhost ,char *outport){
	

	// resolve host/port
	if ((outhost != NULL) && (outport != NULL)) {
		int res;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		if ((res = getaddrinfo(outhost, outport, &hints, &outaddrs)) != 0) {
			fprintf(stderr, "Unable to resolve requested address: %s\n", gai_strerror(res));
			exit(1);
		}
		
		// open output socket
		outfd = socket(outaddrs->ai_family, outaddrs->ai_socktype, outaddrs->ai_protocol);
		if (outfd < 0) {
			fprintf(stderr, "Failed to open output socket\n");
			exit(1);
		}

	}
	srandom(time(NULL));
	
	rtpseq = random();
	ssrc = random();
	
	
	
}


float calculate_fps(RK_U32 *frame_count){
	
	static RK_S64 last_time = 0;
	static RK_S64 last_count = 0;
	RK_S64 now = mpp_time();
	RK_S64 elapsed = now - last_time;
	static float fps=0;
	
	if (*frame_count == 0) {
							last_time = mpp_time();
							last_count = 0;
							mpp_log("init\n");
	} else {
		//mpp_log("elapsed %d\n",elapsed);
		if (elapsed >= 1000000) {
			RK_S64 frames = *frame_count - last_count;
			fps = (float)frames * 1000000 / elapsed;
						
			//mpp_log("decoded %10lld frame %7.2f fps\n",
			//	*frame_count, fps);

			last_time = mpp_time();
			last_count = *frame_count;
		}
	}
	
	return fps;

}	

void frame_change(MppFrame* frame){
	
	// init buffer
	for(int i=0;i<BUFFER_SIZE;i++)
		frame_init(frame,&buffer[i],FRAME_WIDTH,FRAME_HEIGHT);
	
}	

void frame_preprocess(MppFrame* frame){
	
	static int p=0;
	
	MppBuffer src_buf=NULL;
	void *src_ptr=NULL;
	MppBuffer dst_buf=NULL;
	void *dst_ptr=NULL;
	

		
		while(buffer_flag[p]){
			//mpp_log("decoder wait buffer\n");
					
		}
			
		RK_U32 src_width = mpp_frame_get_width(frame);
		RK_U32 src_height = mpp_frame_get_height(frame);
		RK_U32 src_hor_stride = mpp_frame_get_hor_stride(frame);
		RK_U32 src_ver_stride = mpp_frame_get_ver_stride(frame);
		RK_U32 dst_width = mpp_frame_get_width(buffer[p]);
		RK_U32 dst_height = mpp_frame_get_height(buffer[p]);
		RK_U32 dst_hor_stride = mpp_frame_get_hor_stride(buffer[p]);
		RK_U32 dst_ver_stride = mpp_frame_get_ver_stride(buffer[p]);
		
		mpp_frame_get_fmt(frame);
		
		src_buf=mpp_frame_get_buffer(frame);
		src_ptr = mpp_buffer_get_ptr(src_buf);
		size_t len  = mpp_buffer_get_size(src_buf);
			
		dst_buf=mpp_frame_get_buffer(buffer[p]);
		dst_ptr = mpp_buffer_get_ptr(dst_buf);
		size_t len2  = mpp_buffer_get_size(dst_buf);
		
		/////////////////////////////////////////////
		rga_info_t src;
		rga_info_t dst;
					
		memset(&src, 0, sizeof(rga_info_t));
		src.fd = -1;
		src.mmuFlag = 1;
		src.virAddr = src_ptr;
					
		memset(&dst, 0, sizeof(rga_info_t));
		dst.fd = -1;
		dst.mmuFlag = 1;
		dst.virAddr = dst_ptr;
		
		rga_set_rect(&src.rect, 0,0,src_width,src_height,src_hor_stride,src_ver_stride,RK_FORMAT_YCbCr_420_SP);
		rga_set_rect(&dst.rect, 0,0,dst_width,dst_height,dst_hor_stride,dst_ver_stride,RK_FORMAT_YCbCr_420_SP);
					
		MPP_RET ret = c_RkRgaBlit(&src, &dst, NULL);
		
		//dump_mpp_frame_to_file(buffer[p], decoder->fp_output);	
		/////////////////////////////////////////////
		//memcpy(dst_ptr, src_ptr, len2);
		mpp_frame_set_eos(buffer[p], mpp_frame_get_eos(frame));
	
		buffer_flag[p]=1;
		p++;
		p=p%BUFFER_SIZE;

}

FILE *ts_fp;

void ts(char *ts){
	
	//fwrite(ts , 1 , 188, ts_fp );
	
	char buf[12 + 188];
	int bufbase = 0;

	buf[0x0] = 0x80;
	buf[0x1] = 0x21;
	buf[0x4] = 0x00; // }
	buf[0x5] = 0x00; // } FIXME: should really be a valid stamp
	buf[0x6] = 0x00; // }
	buf[0x7] = 0x00; // }
	buf[0x8] = ssrc >> 24;
	buf[0x9] = ssrc >> 16;
	buf[0xa] = ssrc >> 8;
	buf[0xb] = ssrc;
	bufbase = 12;
	
	buf[2] = rtpseq >> 8;
	buf[3] = rtpseq;
	
	memcpy(buf+bufbase , ts , 188);
	
	if (sendto(outfd, buf, bufbase + 188, 0, outaddrs->ai_addr, outaddrs->ai_addrlen) < 0) {
		if (errno != EINTR) {
			fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!Socket send failure: %m !!!!!!!!!!!!!!\n");
				
		}
	}
    usleep(100);
			
	rtpseq++;
	
	//printf("num %d\n",ts[3]);
	
	
	
}	

void frame_packet(MppPacket* packet){
	
	
	
	h264_to_ts(packet,encoder->packet_header,*ts);
	//encoder_write_packet(encoder->fp_output,&packet);
		 
}	

void *thread_input(void *arg)
{

	RK_U32 pkt_eos = 0;
	int count=0;
	int ret;

	do {

		RK_U32 pkt_done = 0;

		pkt_eos=decoder_read_ts_packet(decoder,program[channel].video_pid);

		if( pkt_eos== -1)
				continue;

		// send packet until it success	
	        do {
			ret =decoder_put_packet(decoder);
			if (MPP_OK == ret)
				pkt_done = 1;
            		else {
                	// if failed wait a moment and retry
               	 		//msleep(5);
				//printf("again\n");
				//printf("again\n");
				//printf("again\n");
				//printf("again\n");
            		}
        	} while (!pkt_done);
		
	}while(1);

}

void *thread_output(void *arg)
{
	
	MPP_RET ret = MPP_OK;
	
	
	do{
	
		decoder_run(decoder,&frame_change,&frame_preprocess);
		//mpp_log("decoder frame:%d \n", decoder->frame_count);		
		
	}while(!decoder->frm_eos);
	
	mpp_log("decoder finish\n");
				
}

void *thread_encoder(void *arg)
{
	
	
	static int p=0;
	MPP_RET ret = MPP_OK;
	
	MppPacket packet = NULL;
	
	while(!buffer_flag[p]){
		//mpp_log("encoder wait buffer\n");
		
	}
	
	encoder_data_init(&encoder,buffer[p],FPS);
	encoder_get_header(encoder);
	encoder->fp_output = fopen("out.h264", "w+b");
	encoder_write_packet(encoder->fp_output,&encoder->packet_header);
	RK_S64 start = mpp_time();
	do{
				
		mpp_log("encoder frame count: %d\n",encoder->frame_count);	
		
		RK_S64  minute = ((mpp_time()-start)/1000000)/60;
		RK_S64  sec = ((mpp_time()-start)/1000000)%60;
		RK_S64  avg_fps = encoder->frame_count/((mpp_time()-start)/1000000);
		
		mpp_log("FPS: %f AVG: %d TIME: %d %d\n",calculate_fps(&encoder->frame_count),avg_fps,minute,sec);
		
		
			encoder_run(encoder,buffer[p],*frame_packet);
			
			buffer_flag[p]=0;
			p++;
			p=p%BUFFER_SIZE;
			
		while(!buffer_flag[p] && !encoder->pkt_eos){
		//mpp_log("encoder wait buffer\n");
		
		}
		
	}while(!encoder->pkt_eos);

		mpp_log("encoder finish\n");			
}	

int main(){


	MPP_RET ret = MPP_OK;

	FILE *fp_input=NULL;
	FILE *fp_output=NULL;
	size_t file_size=0;

	pthread_attr_t attr;
	pthread_t thd_in;
	pthread_t thd_out;
	pthread_t thd_encoder;

	//////////////////////

	char frontend_devname[]="/dev/dvb/adapter0/frontend0";
	char demux_devname[]="/dev/dvb/adapter0/demux0";
	char *chanfile = "channels.conf";
	FILE *channel_file = fopen(chanfile, "r");

	FILE *pFile;
	
	pFile = fopen( "demo.h264","w" );
	
	ts_fp=fopen( "out.ts" , "w" );
	connect_init("192.168.1.235" ,"8888");


	int channels_nums = dvbcfg_zapchannel_parse(channel_file,&program);

	 dvb_open_device(frontend_devname,demux_devname,&device);

	if(ret) {
        	printf("open device faild \n");
        	return 1;
	}
 
	ret=ioctl(device->demux_fd, DMX_SET_BUFFER_SIZE,20000000);

         if(ret!=0) {
                printf("set buffer faild \n");
                return 1;
        }

	ret = dvb_open_chennel(device,&program[channel]);

	if(ret) {
        	printf("open channel faild \n");
        	return 1;
	}


	////////////////
	//file_size=read_file("../sample/demo.h264",&fp_input);
	//printf("size: %d\n",file_size);

	if(!decoder_data_init(&decoder))
		return -1;

	fp_output=fopen("out.yuv", "w+b");

	decoder_set_input(decoder,device->demux_fd);
	decoder_set_output(decoder,fp_output);

	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&thd_in, &attr, thread_input, NULL);
    if (ret) {
        mpp_err("failed to create thread for input ret %d\n", ret);
        return 1;
    }

    ret = pthread_create(&thd_out, &attr, thread_output, NULL);
    if (ret) {
        mpp_err("failed to create thread for output ret %d\n", ret);
        return 1;
    }

	ret = pthread_create(&thd_encoder, &attr, thread_encoder, NULL);
    if (ret) {
        mpp_err("failed to create thread for output ret %d\n", ret);
        return 1;
    }


	msleep(500);
    // wait for input then quit decoding
    mpp_log("*******************************************\n");
    mpp_log("**** Press Enter to stop loop decoding ****\n");
    mpp_log("*******************************************\n");
    getc(stdin);



	return 1;
}
