#include"ts_packet.h"
#include <stdio.h>
#include <string.h>

int is_continuous_zero(unsigned char *buf,int p){
	
	if(buf[p] == 0x00 && buf[p+1] == 0x00 ){
		
		
		if(buf[p+2] == 0x01 )
			return p+3;
		else if(buf[p+2] == 0x00 && buf[p+3] == 0x01)
			return p+4;
	
	}
	
	return -1;
}


int ts_packet_init(unsigned char *buf,Ts_packet **packet){

	Ts_packet *p=malloc(sizeof(Ts_packet));
	memset(p,0,sizeof(Ts_packet));
	p->ts_header.transport_error_indicator=(buf[1]&0x80) >> 7;	
	p->ts_header.payload_unit_start_indicator=(buf[1]&0x40) >> 6;
	p->ts_header.transport_priority=(buf[1]&0x20) >> 5;
	p->ts_header.pid=(buf[1]&0x1f);
	p->ts_header.pid=p->ts_header.pid<<8;
	p->ts_header.pid=p->ts_header.pid^buf[2];
	p->ts_header.transport_scrambling_control=(buf[3]&0xC0) >> 6;
	p->ts_header.adaptation_field_control=(buf[3]&0x30) >> 4;
	p->ts_header.continuity_counter=buf[3]&0x0f;
	p->adapt.flag=0;
	p->pes_header.flag=0;
	p->payload.flag=0;
	
	int flag_adapt=(p->ts_header.adaptation_field_control&0x02) >> 1;
	int flag_payload=p->ts_header.adaptation_field_control&0x01;
	
	if(p->ts_header.transport_error_indicator==1)
		return -1;
	
	if(flag_adapt){
		
		p->adapt.flag=1;
		p->adapt.adaptation_field_length=buf[4];
		p->adapt.flag_pcr=buf[5];
		
		if(p->adapt.flag_pcr == 0x50){
			
			p->adapt.PCR[0]=buf[6];
			p->adapt.PCR[1]=buf[7];
			p->adapt.PCR[2]=buf[8];
			p->adapt.PCR[3]=buf[9];
			p->adapt.PCR[4]=buf[10];
		}
			
	}
	
	if(flag_payload){
		
		int payload_offset=4;
		
		if(flag_adapt)
			payload_offset=payload_offset+p->adapt.adaptation_field_length+1;
		
		int zero_p=is_continuous_zero(buf,payload_offset);
		//printf("zero_p= %d\n",zero_p);	
		
		if(zero_p>0){
			
			int stream_id=buf[zero_p];
		
			if(stream_id>=0xc0 && stream_id<=0xef){
				p->pes_header.flag=1;
				p->pes_header.stream_id=stream_id;
				p->pes_header.packet_length=buf[zero_p+1];
				p->pes_header.packet_length=p->pes_header.packet_length<<8;
				p->pes_header.packet_length=p->pes_header.packet_length^buf[zero_p+2];
				p->pes_header.flag_priority=buf[zero_p+3];
				p->pes_header.flag_pts=buf[zero_p+4];
				p->pes_header.data_length=buf[zero_p+5];
				payload_offset=zero_p+5+p->pes_header.data_length+1;
				if(p->pes_header.flag_pts ==0x80 || p->pes_header.flag_pts ==0xc0){
					
					p->pes_header.pts[0]=buf[zero_p+6];
					p->pes_header.pts[1]=buf[zero_p+7];
					p->pes_header.pts[2]=buf[zero_p+8];
					p->pes_header.pts[3]=buf[zero_p+9];
					p->pes_header.pts[4]=buf[zero_p+10];
					
					if(p->pes_header.flag_pts ==0xc0) {
						
						p->pes_header.dts[0]=buf[zero_p+11];
						p->pes_header.dts[1]=buf[zero_p+12];
						p->pes_header.dts[2]=buf[zero_p+13];
						p->pes_header.dts[3]=buf[zero_p+14];
						p->pes_header.dts[4]=buf[zero_p+15];
						
					}
					
				}			
				      
			}
		}
		
		/////////////
		
		p->payload.flag=1;
		p->payload.payload_offset=payload_offset;
		p->payload.size=TS_PACKET_SIZE-payload_offset;
		//printf("SYNC %x %x %x  PID: %d payload.size %d\n",buf[0],buf[1],buf[2],p->ts_header.pid,p->payload.size);	
		
		int i;
		
		for(i=0;i<p->payload.size;i++){
			
			p->payload.data[i]=buf[payload_offset+i];
			
		}
	}
	
	
	
	
	*packet=p;
	
	return 1;

}
