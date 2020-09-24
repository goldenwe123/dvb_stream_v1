#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"

int frame_copy(MppFrame* src_frame,MppFrame* dst_frame);

int frame_init(MppFrame* src_frame,MppFrame* dst_frame,RK_U32 w,RK_U32 h);
