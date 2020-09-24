#include"ts_packet.h"
#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

#define MPI_DEC_STREAM_SIZE         (SZ_4K)

typedef struct
{
	
	
    MppCtx          ctx;
    MppApi          *mpi;
	
	RK_U32 width;
	RK_U32 height;

	MppPacket       packet;
	size_t          packet_size;
	RK_U64			packet_count;
	RK_U32 pkt_eos;
	
	MppBufferGroup  frm_grp;
	MppFrame        frame;
	RK_U64          frame_count;
	
    /* buffer for stream data reading */
	char            *buf;
	RK_U64 			frm_eos;

	FILE            *fp_input;
    FILE            *fp_output;
	
	
} DecoderData;

size_t decoder_read_file(char *name,FILE **fp_input);

int decoder_data_init(DecoderData **data);

void decoder_data_deinit(DecoderData *data);

void decoder_set_input(DecoderData *data,FILE *fp_input);

void decoder_set_output(DecoderData *data,FILE *fp_output);

int decoder_put_packet(DecoderData *data);

int decoder_read_packet(DecoderData *data);

int decoder_run(DecoderData *data,void (*func_frame_change)(MppFrame*),void (*func_frame_get)(MppFrame*));

int decoder_read_ts_packet(DecoderData *data,int pid);
