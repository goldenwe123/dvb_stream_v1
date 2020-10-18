#include "dvbcfg_common.h"
#include "gn_dvb.h"

static const struct dvbcfg_setting dvbcfg_inversion_list[] = {
	{ "INVERSION_ON",INVERSION_ON },
	{ "INVERSION_OFF",INVERSION_OFF },
	{ "INVERSION_AUTO",INVERSION_AUTO },
	{ NULL, 0 }
};
static const struct dvbcfg_setting dvbcfg_fec_list[] = {
	{ "FEC_1_2", FEC_1_2},
	{ "FEC_2_3", FEC_2_3},
	{ "FEC_3_4", FEC_3_4},
	{ "FEC_4_5", FEC_4_5},
	{ "FEC_5_6", FEC_5_6},
	{ "FEC_6_7", FEC_6_7},
	{ "FEC_7_8", FEC_7_8},
	{ "FEC_8_9", FEC_8_9},
	{ "FEC_AUTO",FEC_AUTO},
	{ "FEC_NONE",FEC_NONE},
	{ NULL, 0 }
};
static const struct dvbcfg_setting dvbcfg_constellation_list[] = {
	{ "QAM_16", QAM_16},
	{ "QAM_32", QAM_32},
	{ "QAM_64", QAM_64},
	{ "QAM_128", QAM_128},
	{ "QAM_256", QAM_256},
	{ "QAM_AUTO", QAM_AUTO},
	{ NULL, 0 }
};
static const struct dvbcfg_setting dvbcfg_transmission_mode_list[] = {
	{ "TRANSMISSION_MODE_2K", TRANSMISSION_MODE_2K },
	{ "TRANSMISSION_MODE_8K", TRANSMISSION_MODE_8K },
	{ "TRANSMISSION_MODE_AUTO", TRANSMISSION_MODE_AUTO },
	{ NULL, 0 }
};
static const struct dvbcfg_setting dvbcfg_guard_interval_list[] = {
	{ "GUARD_INTERVAL_1_32", GUARD_INTERVAL_1_32 },
	{ "GUARD_INTERVAL_1_16", GUARD_INTERVAL_1_16 },
	{ "GUARD_INTERVAL_1_8", GUARD_INTERVAL_1_8 },
	{ "GUARD_INTERVAL_1_4", GUARD_INTERVAL_1_4 },
	{ "GUARD_INTERVAL_AUTO", GUARD_INTERVAL_AUTO },
	{ NULL, 0 }
};
static const struct dvbcfg_setting dvbcfg_hierarchy_list[] = {
	{ "HIERARCHY_1", HIERARCHY_1 },
	{ "HIERARCHY_2", HIERARCHY_2 },
	{ "HIERARCHY_4", HIERARCHY_4 },
	{ "HIERARCHY_AUTO", HIERARCHY_AUTO },
	{ "HIERARCHY_NONE", HIERARCHY_NONE },
	{ NULL, 0 }
};
static const struct dvbcfg_setting dvbcfg_bandwidth_list[] = {
	{ "BANDWIDTH_6_MHZ", 6000000 },
	{ "BANDWIDTH_7_MHZ", 7000000 },
	{ "BANDWIDTH_8_MHZ", 8000000 },
	{ NULL, 0 }
};

int dvbcfg_zapchannel_parse(FILE *file,struct gn_program **program )
{
	char *line_buf = NULL;
	size_t line_size = 0;
	int line_len = 0;
	int ret_val = 0;
	int count = 0;
	struct gn_program p[MAX_PROGRAM_NUMS];

        while ((line_len = getline(&line_buf, &line_size, file)) > 0 && count < MAX_PROGRAM_NUMS ) {
                char *line_tmp = line_buf;
                char *line_pos = line_buf;

                /* remove newline and comments (started with hashes) */
                while ((*line_tmp != '\0') && (*line_tmp != '\n') && (*line_tmp != '#'))
                        line_tmp++;
                *line_tmp = '\0';

                /* parse name */
                dvbcfg_parse_string(&line_pos, ":", p[count].name, sizeof(p[count].name));
                printf("Name: %s\n",p[count].name);
                if (!line_pos)
                        continue;

                /* parse frequency */
				p[count].property[0].cmd = DTV_FREQUENCY;
                p[count].property[0].u.data = dvbcfg_parse_int(&line_pos, ":");
                printf("frequence: %d \n",p[count].property[0].u.data);
                if (!line_pos)
                        continue;

                /* parse frontend specific settings */

                /* inversion */
		p[count].property[1].cmd = DTV_INVERSION;
		p[count].property[1].u.data = dvbcfg_parse_setting(&line_pos, ":", dvbcfg_inversion_list);
		printf("Inversion: %d\n", p[count].property[1].u.data);
		if (!line_pos)
			continue;

		/* bandwidth */
		p[count].property[2].cmd = DTV_BANDWIDTH_HZ;
		p[count].property[2].u.data= dvbcfg_parse_setting(&line_pos, ":", dvbcfg_bandwidth_list);
		printf("freq: %d\n",p[count].property[2].u.data);
		if (!line_pos)
			continue;

		/* fec hp */
		p[count].property[3].cmd = DTV_INNER_FEC;
		p[count].property[3].u.data = dvbcfg_parse_setting(&line_pos, ":", dvbcfg_fec_list);
		printf("hp: %d\n",p[count].property[3].u.data);
		if (!line_pos)
			continue;

		/* fec lp */

		int lp =
			dvbcfg_parse_setting(&line_pos, ":", dvbcfg_fec_list);
		//printf("lp: %d\n",lp);
		if (!line_pos)
			continue;

		/* constellation */
		p[count].property[4].cmd = DTV_MODULATION;
		p[count].property[4].u.data =	dvbcfg_parse_setting(&line_pos, ":", dvbcfg_constellation_list);
		printf("constellation: %d\n", p[count].property[4].u.data);
		if (!line_pos)
			continue;

		/* transmission mode */
		p[count].property[5].cmd = DTV_TRANSMISSION_MODE;
		p[count].property[5].u.data =dvbcfg_parse_setting(&line_pos, ":", dvbcfg_transmission_mode_list);
		printf("transmission: %d\n",  p[count].property[5].u.data);
		if (!line_pos)
			continue;

		/* guard interval */
		p[count].property[6].cmd = DTV_GUARD_INTERVAL;
		p[count].property[6].u.data = dvbcfg_parse_setting(&line_pos, ":", dvbcfg_guard_interval_list);
		printf("guard_interval: %d\n",  p[count].property[6].u.data);
		if (!line_pos)
			continue;

		/* hierarchy */
		p[count].property[7].cmd = DTV_HIERARCHY;
		p[count].property[7].u.data = dvbcfg_parse_setting(&line_pos, ":", dvbcfg_hierarchy_list);
		printf("hierarchy: %d\n", p[count].property[7].u.data);
		if (!line_pos)
			continue;

		p[count].property[8].cmd = DTV_DELIVERY_SYSTEM;
		p[count].property[8].u.data = SYS_DVBT;
		p[count].property[9].cmd = DTV_TUNE;
		p[count].property_len = 10;

		/* parse video and audio pids and service id */
		p[count].video_pid = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos)
			continue;
		p[count].audio_pid = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos)
			continue;
		p[count].service_id = dvbcfg_parse_int(&line_pos, ":");
		if (!line_pos) /* old files don't have a service id */
			p[count].service_id = 0;

		count++;
	}
	*program=p;

	return count;
}

int dvb_open_device(char *frontend_devname,char *demux_devname,struct gn_dvb **device)
{


	struct gn_dvb *p;
	p=malloc(sizeof(p));


	if (( p->frontend_fd = open(frontend_devname, O_RDWR)) < 0) {
                printf("open frontend  DEVICE ERROR\n");
                return 1;
        }

        if (( p->demux_fd = open(demux_devname, O_RDONLY)) < 0) {
                printf("open demux DEVICE ERROR\n");
                return 1;
        }

	*device=p;

	return 0;
}

int dvb_close_device(struct gn_dvb *device)
{

	if ( close(device->frontend_fd) < 0) {
                printf("close frontend  DEVICE ERROR\n");
                return 1;
        }

        if (close(device->demux_fd) < 0) {
                printf("close demux DEVICE ERROR\n");
                return 1;
        }

        return 0;
}

int dvb_open_chennel(struct gn_dvb *device,struct gn_program *program)
{

	int ret;

	struct dtv_properties cmdseq = {.num = program->property_len, .props = program->property};
	ret=ioctl(device->frontend_fd, FE_SET_PROPERTY, &cmdseq);

	if(ret) {
		printf("set frontend failed\n");
		return 1;
	}

	struct dmx_pes_filter_params f;
	f.pid = (u_int16_t)program->video_pid;
	f.output = DMX_OUT_TSDEMUX_TAP;
	f.input = DMX_IN_FRONTEND;
	f.pes_type = DMX_PES_OTHER;

  	if (ioctl(device->demux_fd, DMX_SET_PES_FILTER, &f) == -1) {
		printf("ioctl DMX_PES_FILTER failed\n");
		return 1;
	}

	unsigned int pid = program->audio_pid;
	ret = ioctl(device->demux_fd, DMX_ADD_PID, &pid);

	pid = 0;
	ret = ioctl(device->demux_fd, DMX_ADD_PID, &pid);
	
	
	pid = 4000;
	ret = ioctl(device->demux_fd, DMX_ADD_PID, &pid);
	
	if(ret) {
		printf("add pid failed\n");
		return 1;
	}

	ret=ioctl(device->demux_fd, DMX_START);
	if(ret) {
		printf("dmx start failed\n");
		return 1;
	}

	return 0;

}


