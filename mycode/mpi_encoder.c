#include "mpi_encoder.h"

MPP_RET encoder_data_init(EncoderData **data,MppFrame* frame,RK_S32 fps)
{
	
	MPP_RET ret = MPP_OK;	
	EncoderData *p = NULL;
	p = mpp_calloc(EncoderData, 1);
	
	if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        return ret;
    }
	
	p->width        = mpp_frame_get_width(frame);
    p->height       = mpp_frame_get_height(frame);
	p->hor_stride   = mpp_frame_get_hor_stride(frame);
    p->ver_stride   = mpp_frame_get_ver_stride(frame);
	
	p->packet	= NULL;
	p->pkt_eos	=0;
	
	p->frame_count	= 0;
	p->stream_size	= 0;
	
	p->fp_input	= NULL;
	p->fp_output	= NULL;
	

	
    p->fmt          = mpp_frame_get_fmt(frame);
    p->type         = MPP_VIDEO_CodingAVC;
	p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
	
	 // update resource parameter
    if (p->fmt <= MPP_FMT_YUV420SP_VU)
        p->frame_size = p->hor_stride * p->ver_stride * 3 / 2;
    else if (p->fmt <= MPP_FMT_YUV422_UYVY) {
        // NOTE: yuyv and uyvy need to double stride
        p->hor_stride *= 2;
        p->frame_size = p->hor_stride * p->ver_stride;
    } else
        p->frame_size = p->hor_stride * p->ver_stride * 4;
	
	p->packet_size  = p->width * p->height;
	
	*data = p;

	ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        return ret;
    }
	
	ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        return ret;
    }
	
	MppEncCodecCfg *codec_cfg;
    MppEncPrepCfg *prep_cfg;
    MppEncRcCfg *rc_cfg;
	MppApi *mpi;
    MppCtx ctx;
	
	ctx = p->ctx;
	mpi = p->mpi;
	codec_cfg = &p->codec_cfg;
    prep_cfg = &p->prep_cfg;
    rc_cfg = &p->rc_cfg;
	
	 /* setup default parameter */
	p->fps = fps;
    p->gop = 30;
    p->bps = p->width * p->height / 8 * p->fps;
	p->bps = 500000;
	prep_cfg->change        = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                              MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                              MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg->width         = p->width;
    prep_cfg->height        = p->height;
    prep_cfg->hor_stride    = p->hor_stride;
    prep_cfg->ver_stride    = p->ver_stride;
    prep_cfg->format        = p->fmt;
    prep_cfg->rotation      = MPP_ENC_ROT_0;
	
    ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
    if (ret) {
        mpp_err("mpi control enc set prep cfg failed ret %d\n", ret);
        return ret;
    }
	
	rc_cfg->change  = MPP_ENC_RC_CFG_CHANGE_ALL;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;
	
	if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) {
        /* constant bitrate has very small bps range of 1/16 bps */
        rc_cfg->bps_target   = p->bps;
        rc_cfg->bps_max      = p->bps * 17 / 16;
        rc_cfg->bps_min      = p->bps * 15 / 16;
    } else if (rc_cfg->rc_mode ==  MPP_ENC_RC_MODE_VBR) {
        if (rc_cfg->quality == MPP_ENC_RC_QUALITY_CQP) {
            /* constant QP does not have bps */
            rc_cfg->bps_target   = -1;
            rc_cfg->bps_max      = -1;
            rc_cfg->bps_min      = -1;
        } else {
            /* variable bitrate has large bps range */
            rc_cfg->bps_target   = p->bps;
            rc_cfg->bps_max      = p->bps * 17 / 16;
            rc_cfg->bps_min      = p->bps * 1 / 16;
        }
    }
	
	/* fix input / output frame rate */
    rc_cfg->fps_in_flex      = 0;
    rc_cfg->fps_in_num       = p->fps;
    rc_cfg->fps_in_denorm    = 0;
    rc_cfg->fps_out_flex     = 0;
    rc_cfg->fps_out_num      = p->fps;
    rc_cfg->fps_out_denorm   = 0;

    rc_cfg->gop              = p->gop;
    rc_cfg->skip_cnt         = 0;

    mpp_log("mpi_enc_test bps %d fps %d gop %d\n",
            rc_cfg->bps_target, rc_cfg->fps_out_num, rc_cfg->gop);
    ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
    if (ret) {
        mpp_err("mpi control enc set rc cfg failed ret %d\n", ret);
        return ret;
    }
	
	codec_cfg->coding = p->type;
	mpp_log("===========================");
    switch (codec_cfg->coding) {
    case MPP_VIDEO_CodingAVC : {
        codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                                 MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                                 MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        codec_cfg->h264.profile  = 100;
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        codec_cfg->h264.level    = 40;
        codec_cfg->h264.entropy_coding_mode  = 1;
        codec_cfg->h264.cabac_init_idc  = 0;
        codec_cfg->h264.transform8x8_mode = 1;
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        codec_cfg->jpeg.change  = MPP_ENC_JPEG_CFG_CHANGE_QP;
        codec_cfg->jpeg.quant   = 10;
    } break;
    case MPP_VIDEO_CodingVP8 : {
    } break;
    case MPP_VIDEO_CodingHEVC : {
        codec_cfg->h265.change = MPP_ENC_H265_CFG_INTRA_QP_CHANGE;
        codec_cfg->h265.intra_qp = 26;
    } break;
    default : {
        mpp_err_f("support encoder coding type %d\n", codec_cfg->coding);
    } break;
    }
    ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
    if (ret) {
        mpp_err("mpi control enc set codec cfg failed ret %d\n", ret);
       return ret;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        return ret;
    }
	
	
	
	return ret;
}

MPP_RET encoder_data_deinit(EncoderData **data)
{
    
	EncoderData *p = NULL;

    if (!data) {
        mpp_err_f("invalid input data %p\n", data);
        return MPP_ERR_NULL_PTR;
    }

    p = *data;
    if (p) {
        if (p->fp_input) {
            fclose(p->fp_input);
            p->fp_input = NULL;
        }
        if (p->fp_output) {
            fclose(p->fp_output);
            p->fp_output = NULL;
        }
        MPP_FREE(p);
        *data = NULL;
    }

    return MPP_OK;
}	

MPP_RET encoder_write_packet(FILE *fp_output,MppPacket* packet){
	
		if (*packet && fp_output) {
            
			void *ptr   = mpp_packet_get_pos(*packet);
            size_t len  = mpp_packet_get_length(*packet);
		
            fwrite(ptr, 1, len, fp_output);

          return 1;
		}
	
	return -1;
}	

MPP_RET encoder_get_header(EncoderData *data)
{
	MPP_RET ret = MPP_OK;
	mpp_log("type: %d\n",data->type);
	switch(data->type) {
		
		case MPP_VIDEO_CodingAVC:
		
			ret = data->mpi->control(data->ctx, MPP_ENC_GET_EXTRA_INFO, &data->packet_header);
	
			if (ret) {
				mpp_err("mpi control enc get extra info failed\n");
				return ret;
			}
		
		break;
		 
	}

	return ret;
	
}	

MPP_RET encoder_run(EncoderData *data,MppFrame* frame,void (*func_ptr)(MppPacket *))
{
	MPP_RET ret = MPP_OK;
	MppFrame frame2 = NULL;
	frame_copy(frame,&frame2);
	
	ret = data->mpi->encode_put_frame(data->ctx, frame2 );

	data->frame_count++;

	do{
		
		ret = data->mpi->encode_get_packet(data->ctx, &data->packet);
			
	}while(!data->packet);
	
	if (data->packet) {
		
		if(*func_ptr)
			(*func_ptr)(data->packet);
		
		data->pkt_eos = mpp_packet_get_eos(data->packet);
		
		mpp_packet_deinit(&data->packet);
		
		
	}
	
	return ret;
}	
