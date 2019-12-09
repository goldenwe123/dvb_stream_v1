#include "mpi_frame_init.h"

int frame_copy(MppFrame* src_frame,MppFrame* dst_frame){
	
		MPP_RET ret;		
		RK_U32 w = mpp_frame_get_width(src_frame);
		RK_U32 h = mpp_frame_get_height(src_frame);
		RK_U32 hor_stride = mpp_frame_get_hor_stride(src_frame);
		RK_U32 ver_stride = mpp_frame_get_ver_stride(src_frame);
		RK_U32 fmt = mpp_frame_get_fmt(src_frame);

		ret = mpp_frame_init(dst_frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            return -1;
        }
			
		mpp_frame_set_width(*dst_frame, w);
		mpp_frame_set_height(*dst_frame, h);
		mpp_frame_set_hor_stride(*dst_frame, hor_stride);
		mpp_frame_set_ver_stride(*dst_frame, ver_stride);
		mpp_frame_set_fmt(*dst_frame, fmt);
		mpp_frame_set_eos(*dst_frame,mpp_frame_get_eos(src_frame));
		size_t frame_size=hor_stride * ver_stride * 3 / 2;
		
		MppBuffer buf=NULL;
		buf =mpp_frame_get_buffer(src_frame);	
		mpp_frame_set_buffer(*dst_frame,buf);
	
	return 1;
}

int frame_init(MppFrame* src_frame,MppFrame* dst_frame,RK_U32 w,RK_U32 h){
	
		MPP_RET ret;		
		RK_U32 hor_stride = MPP_ALIGN(w, 8);
		RK_U32 ver_stride = MPP_ALIGN(h, 8);
		RK_U32 fmt = mpp_frame_get_fmt(src_frame);

		ret = mpp_frame_init(dst_frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            return -1;
        }
			
		mpp_frame_set_width(*dst_frame, w);
		mpp_frame_set_height(*dst_frame, h);
		mpp_frame_set_hor_stride(*dst_frame, hor_stride);
		mpp_frame_set_ver_stride(*dst_frame, ver_stride);
		mpp_frame_set_fmt(*dst_frame, fmt);
		mpp_frame_set_eos(*dst_frame,mpp_frame_get_eos(src_frame));
		size_t frame_size=hor_stride * ver_stride * 3 / 2;
		
		MppBuffer buf=NULL;
		ret = mpp_buffer_get(NULL, &buf,frame_size);	
		if (ret) {
			mpp_err("failed to get buffer for input frame ret %d\n", ret);
			return -1;
		}
			
		mpp_frame_set_buffer(*dst_frame,buf);
	
	return 1;
}		