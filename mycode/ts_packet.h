#include <stdio.h>
#include <stdlib.h>

#define TS_PACKET_SIZE 188
#define TS_SYNC_BYTE 0x47

typedef struct {
	
	unsigned char sync_byte;
	unsigned char transport_error_indicator;
	unsigned char payload_unit_start_indicator;
	unsigned char transport_priority;
	unsigned short pid;
	unsigned char transport_scrambling_control;
	unsigned char adaptation_field_control;
	unsigned char continuity_counter ;

}Ts_header;

typedef struct {
	
	unsigned char flag;
	unsigned char adaptation_field_length;
	unsigned char flag_pcr; //0x50 have pcr 0x40 no pcr 
	unsigned char PCR[5];
	
}Adaptation;

typedef struct {
	
	unsigned char flag;
	unsigned char stream_id;
	unsigned short packet_length;
	unsigned char flag_priority;
	unsigned char flag_pts;
	unsigned char flag_dts;
	unsigned char data_length;
	unsigned char pts[5];
	unsigned char dts[5];

}Pes_header;

typedef struct {
	
	unsigned char flag;
	size_t size;
	int payload_offset;
	unsigned char data[TS_PACKET_SIZE];
	
}Payload;

typedef struct {
	
	Ts_header	ts_header;
	Adaptation	adapt;
	Pes_header	pes_header;
	Payload	payload;

}Ts_packet;

int ts_packet_init(unsigned char *buf,Ts_packet **packet);
