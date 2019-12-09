#include <linux/dvb/dmx.h> 
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define MAX_PROGRAM_NUMS 50

struct gn_dvb{

	int frontend_fd;
	int demux_fd;

};


struct gn_program{

	char name[50];
	int video_pid;
	int audio_pid;
	int service_id;
	int property_len;
	struct dtv_property property[10];

};

int dvbcfg_zapchannel_parse(FILE *file,struct gn_program **program );

int dvb_open_device(char *frontend_devname,char *demux_devname,struct gn_dvb **device);

int dvb_close_device(struct gn_dvb *device);

int dvb_open_chennel(struct gn_dvb *device,struct gn_program *program);
