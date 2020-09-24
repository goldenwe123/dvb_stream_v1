

#include "mpi_decoder.h"


size_t decoder_read_file(char *name,FILE **fp_input){
	
	
	size_t file_size;
	*fp_input=fopen(name, "rb");
	
	if (NULL == *fp_input) {
		mpp_err("failed to open input file %s\n", name);
		return -1;
	}
	
	fseek(*fp_input, 0L, SEEK_END);
	file_size = ftell(*fp_input);
	rewind(*fp_input);
	
	return file_size;
}

int decoder_data_init(DecoderData **data){
	
	MPP_RET ret         = MPP_OK;
	MpiCmd mpi_cmd      = MPP_CMD_BASE;
	MppCodingType type  = MPP_VIDEO_CodingAVC;
    MppParam param      = NULL;
    RK_U32 need_split   = 1;
    MppPollType timeout = -1;
	
	DecoderData *p = NULL;
	p = mpp_calloc(DecoderData, 1);
	
	// base flow context
    p->ctx         = NULL;
    p->mpi         = NULL;
	
	p->width=0;
	p->height=0;
	
	p->fp_input=NULL;
	p->fp_output=NULL;
	
    // input / output
    p->packet    = NULL;
	p->packet_size=MPI_DEC_STREAM_SIZE;
	p->packet_count	=0;
	
	p->frm_grp	=NULL;
    p->frame     = NULL;
	p->frame_count=0;
	
	// resources
	p->frm_eos		=0;
   	
	// paramter for resource malloc
	p->buf = mpp_malloc(char,p->packet_size);

    if (NULL == p->buf) {
        mpp_err("mpi_dec_test malloc input stream buffer failed\n");
        return -1;
    }
	
	ret = mpp_packet_init(&p->packet,p->buf,p->packet_size);
    if (ret) {
        mpp_err("mpp_packet_init failed\n");
        return -1;
    }
	
	// decoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (MPP_OK != ret) {
        mpp_err("mpp_create failed\n");
        return -1;
    }
	
	// NOTE: decoder split mode need to be set before init
    mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    param = &need_split;
    ret = p->mpi->control(p->ctx, mpi_cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        return -1;
    }
	
	// NOTE: timeout value please refer to MppPollType definition
    //  0   - non-block call (default)
    // -1   - block call
    // +val - timeout value in ms
	
	if (timeout) {
        param = &timeout;
        ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, param);
        if (MPP_OK != ret) {
            mpp_err("Failed to set output timeout %d ret %d\n", timeout, ret);
            return -1;
        }
    }
	
	ret = mpp_init(p->ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        return -1;
    }
	
	*data = p;
	
	return 1;
	
}	

void decoder_data_deinit(DecoderData *data){
	
	if (data->packet) {
        mpp_packet_deinit(&data->packet);
        data->packet = NULL;
    }

    if (data->frame) {
        mpp_frame_deinit(&data->frame);
        data->frame = NULL;
    }

    if (data->ctx) {
        mpp_destroy(data->ctx);
        data->ctx = NULL;
    }
	
	if (data->buf) {
        mpp_free(data->buf);
        data->buf = NULL;
    }
	
	if (data->frm_grp) {
        mpp_buffer_group_put(data->frm_grp);
        data->frm_grp = NULL;
    }
	
	if (data->fp_output) {
        fclose(data->fp_output);
        data->fp_output = NULL;
    }

    if (data->fp_input) {
        fclose(data->fp_input);
        data->fp_input = NULL;
    }
		
	return;
}

void decoder_set_input(DecoderData *data,FILE *fp_input){
	
	data->fp_input=fp_input;
	
	return;
}

void decoder_set_output(DecoderData *data,FILE *fp_output){

	data->fp_output=fp_output;
	
	return;
}
		
int decoder_put_packet(DecoderData *data){
	
	MPP_RET ret = MPP_OK;
	
	ret =data->mpi->decode_put_packet(data->ctx,data->packet);

	return ret;
}	
		
int decoder_read_packet(DecoderData *data){
	
	//mpp_log("read packet\n");
	data->pkt_eos=0;
	size_t read_size=0; 
	read_size	= fread(data->buf, 1, data->packet_size, data->fp_input);
	//mpp_log("read_size %d\n",read_size);
	
	if (read_size != data->packet_size || feof(data->fp_input)) {
		// setup eos flag
		data->pkt_eos = 1;

		// reset file to start and clear eof / ferror
		clearerr(data->fp_input);
		rewind(data->fp_input);
	}
	data->packet_count++;
	
		// write data to packet
        mpp_packet_write(data->packet, 0,data->buf, read_size);
		// reset pos and set valid length
        mpp_packet_set_pos(data->packet, data->buf);
        mpp_packet_set_length(data->packet, read_size);
		
		if (data->pkt_eos) {
            mpp_packet_set_eos(data->packet);
            mpp_log("found last packet\n");
        } else
            mpp_packet_clr_eos(data->packet);
		
		
	return data->pkt_eos;	
	
		
}

int decoder_read_ts_packet(DecoderData *data,int pid){

/*
	int num=read( data->fp_input,data->buf,188);

	if(num!=188) {
		printf("***num %d %x\n",num,data->buf[0]);
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");
		printf("*****************************\n");

		//usleep(1000000);
	}
	if(num>0) {

		Ts_packet *packet;
		int ret=ts_packet_init(data->buf,&packet);
		//printf("packet size %d \n",packet->payload.size);
		if(ret>0) {
				if(packet->ts_header.pid==pid) {
					data->packet_count++;
					//printf("packet size %d \n",packet->payload.size);
					mpp_packet_write(data->packet, 0,&packet->payload.data[0],packet->payload.size);
					mpp_packet_set_pos(data->packet, &packet->payload.data[0]);
					mpp_packet_set_length(data->packet, packet->payload.size);
					//mpp_packet_clr_eos(data->packet);
					free(packet);
					packet=NULL;

					return 0;
				}
		}

		free(packet);
		packet=NULL;
	}
*/

	return -1;
}

int  decoder_run(DecoderData *data,void (*func_frame_change)(MppFrame*),void (*func_frame_get)(MppFrame*)){
	
	MPP_RET ret=MPP_OK;
	
	ret = data->mpi->decode_get_frame(data->ctx, &data->frame);
	
	if (data->frame) {
		
		if (mpp_frame_get_info_change(data->frame)) {
			
			data->width=mpp_frame_get_width(data->frame);
			data->height=mpp_frame_get_height(data->frame);
			RK_U32 buf_size = mpp_frame_get_buf_size(data->frame);

			if (NULL == data->frm_grp) {
				/* If buffer group is not set create one and limit it */
				ret = mpp_buffer_group_get_internal(&data->frm_grp, MPP_BUFFER_TYPE_ION);
				if (ret) {
					mpp_err("get mpp buffer group failed ret %d\n", ret);
					return -1;
				}

				/* Set buffer to mpp decoder */
				ret = data->mpi->control(data->ctx, MPP_DEC_SET_EXT_BUF_GROUP, data->frm_grp);
				if (ret) {
					mpp_err("set buffer group failed ret %d\n", ret);
					return -1;
				}
			} else {
				/* If old buffer group exist clear it */
				ret = mpp_buffer_group_clear(data->frm_grp);
				if (ret) {
					mpp_err("clear buffer group failed ret %d\n", ret);
					return -1;
				}
			}
					
			/* Use limit config to limit buffer count to 24 with buf_size */
			ret = mpp_buffer_group_limit_config(data->frm_grp, buf_size, 24);
			if (ret) {
				mpp_err("limit buffer group failed ret %d\n", ret);
				return -1;
			}
						
					
			ret = data->mpi->control(data->ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
			if (ret) {
				mpp_err("info change ready failed ret %d\n", ret);
				return -1;
			}
			
			if(*func_frame_change)
				(*func_frame_change)(data->frame);
		
			
			
		}else {
			
			
			if(*func_frame_get)
				(*func_frame_get)(data->frame);
			
			
			data->frm_eos=mpp_frame_get_eos(data->frame);
	
			data->frame_count++;
		}	
	
	
	
			mpp_frame_deinit(&data->frame);
			data->frame = NULL;
	
	}
	
}	

	

