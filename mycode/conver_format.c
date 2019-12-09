#include"conver_format.h"
#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

void get_pts(unsigned char *p){
	
	float frame_rate=30;
	float pts_offset=90000/frame_rate;
	static unsigned long pts=0;
	
	pts = pts + pts_offset;
	
	p[0] = 0x21 | ((pts >> 29) & 0x0E);
	p[1] = (pts >>22 & 0xFF);
	p[2] =	0x01 | ((pts >> 14 ) & 0xFE);
	p[3] = (pts >> 7 & 0xFF);
	p[4] =0x01 | ((pts << 1 ) & 0xFE);
	
}


int put_ts_header(char *packet,Ts_header *ts){
	
	
	char temp;
	static char count=0;
	count=(count + 1) & 0x0f;
	ts->continuity_counter = count;
	//printf("count: %x\n",count);
	packet[0]=ts->sync_byte;
	
	temp = ( ts->transport_error_indicator << 7 ) + ( ts->payload_unit_start_indicator << 6 )
				+ ( ts->transport_priority << 5 ) + ( ts->pid >> 8 );
	
	packet[1]=temp;
	
	temp = ts->pid & 0xfff;
	
	packet[2]=temp;
	
	temp = ( ts->transport_scrambling_control << 6 ) + ( ts->adaptation_field_control << 4 )+ ts->continuity_counter;
	
	packet[3]=temp;
	//printf("packet[3]: %x\n",packet[3]);
	
	return 4;

}

int put_pes_header(char *packet,Pes_header *pes){
	
	char temp;
	temp=0x00;
	packet[0]=temp;
	temp=0x00;
	packet[1]=temp;
	temp=0x01;
	packet[2]=temp;
	temp=pes->stream_id;
	packet[3]=temp;
	temp=0x00;
	packet[4]=temp;
	temp=0x00;
	packet[5]=temp;
	temp=pes->flag_priority;
	packet[6]=temp;
	temp=0x80;
	packet[7]=temp;
	temp=pes->data_length;
	packet[8]=temp;

	packet[9]=pes->pts[0];
	packet[10]=pes->pts[1];
	packet[11]=pes->pts[2];
	packet[12]=pes->pts[3];
	packet[13]=pes->pts[4];
	
	temp=0x00;
	packet[14]=temp;
	temp=0x00;
	packet[15]=temp;
	temp=0x00;
	packet[16]=temp;
	temp=0x01;
	packet[17]=temp;
	temp=0x09;
	packet[18]=temp;
	temp=0xf0;
	packet[19]=temp;
	
	

	return 20;
}

void put_data(char *packet,char *data,size_t len){
	
	for(int i=0;i<len;i++)		
			packet[i] = data[i];	
}	


void h264_to_ts(MppPacket* packet,MppPacket* header,void (*func_ptr)(char*)){
	
	Ts_header ts_header={
	
		.sync_byte =  0x47,
		.transport_error_indicator = 0,
		.payload_unit_start_indicator = 0,
		.transport_priority = 0,
		.pid = 4001,
		.transport_scrambling_control = 0,
		.adaptation_field_control = 1,
		.continuity_counter = 0
	};	

	Pes_header pes_header={
	
		.flag = 1,
		.stream_id = 0xe0,
		.packet_length = 0,
		.flag_priority = 0x80,
		.flag_pts = 1,
		.flag_dts = 0,
		.data_length = 5,
	
	};
	
	
	
	char * header_ptr   = mpp_packet_get_pos(header);	
	size_t header_len  = mpp_packet_get_length(header);

	char *data_ptr   = mpp_packet_get_pos(packet);	
	size_t data_len  = mpp_packet_get_length(packet);
	
	char *pes_ptr[20];
	size_t pes_len=20;
	
	get_pts(pes_header.pts);
	put_pes_header(pes_ptr,&pes_header);
	
	char *ptr;	
	size_t len;
	
	
	if(data_ptr[4]==0x25){
	
		len = pes_len+header_len+data_len;
		//printf("len: %d %d %d %d\n",pes_len,header_len,data_len,len);
		ptr = malloc(len);
		memcpy(ptr , pes_ptr , pes_len);
		memcpy(ptr+pes_len , header_ptr , header_len);
		memcpy(ptr+pes_len+header_len , data_ptr , data_len);
		
	}else{
		len = pes_len+data_len;
		
		ptr = malloc(len);
		memcpy(ptr,pes_ptr,pes_len);
		memcpy(ptr+pes_len,data_ptr,data_len);
		
	}

	char buffer[TS_PACKET_SIZE]={0};

	int count=0;
	int p=0;
	int num=0;
	
	
	while(p<len){
		
		int offset=0;
		
		count++;
		if(count==1){
			ts_header.payload_unit_start_indicator = 1;
		}else{
			ts_header.payload_unit_start_indicator = 0;
		}	
		
		
		int stuff_nums = TS_PACKET_SIZE - 4 - (len - p);
		
		if(stuff_nums>0){
			
			ts_header.adaptation_field_control = 3;
		
			
		}else{
			ts_header.adaptation_field_control = 1;
		}
		
		offset += put_ts_header(buffer+offset,&ts_header);

		if(stuff_nums >0){
			
			buffer[offset++] = stuff_nums - 1;

			if(stuff_nums!=1)
				buffer[offset++] = 0;
					
			for(int j=0;j<(stuff_nums-2);j++)
				buffer[offset++] = 0xff;
		}
		
		num=min(TS_PACKET_SIZE - offset, len - p);
	
		for(int i=p;i<=p+num;i++)
			buffer[offset++] = ptr[i];

		(*func_ptr)(buffer);
		
		p=p+num;
		
	}	
	free(ptr);
	
	return;
}	


