#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpi_frame_init.h"

typedef struct {
	
	RK_U32 width;
    RK_U32 height;
	RK_U32 hor_stride;
    RK_U32 ver_stride;
	
	MppPacket packet;
	MppPacket packet_header;
	RK_U32 pkt_eos;
	
    RK_U32 frame_count;
    RK_U64 stream_size;

    // src and dst
    FILE *fp_input;
    FILE *fp_output;
	
	// rate control runtime parameter
    RK_S32 gop;
    RK_S32 fps;
    RK_S32 bps;

    // paramter for resource malloc
    MppFrameFormat fmt;
    MppCodingType type;
	
	// input / output
    MppEncSeiMode sei_mode;
	
	// base flow context
    MppCtx ctx;
    MppApi *mpi;
	MppEncCodecCfg codec_cfg;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    
    // resources
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;


	
} EncoderData;

MPP_RET frame_copy(MppFrame* src_frame,MppFrame* dst_frame);

MPP_RET encoder_data_init(EncoderData **data,MppFrame* frame,RK_S32 fps);

MPP_RET encoder_data_deinit(EncoderData **data);

MPP_RET encoder_get_header(EncoderData *data);

MPP_RET encoder_write_packet(FILE *fp_output,MppPacket* packet);

MPP_RET encoder_run(EncoderData *data,MppFrame* frame,void (*func_ptr)(MppPacket*));

