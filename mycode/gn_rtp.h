#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define RTP_BUFFER_NUMS 1500
#define RTP_CLIENT "192.168.1.234"
#define RTP_PORT "8888" 

void gn_rtp_connect_init(char *outhost ,char *outport);

void gn_rtp_put(char* data);

void gn_rtp_send();

void gn_rtp_audio_put(char* data);

void gn_rtp_audio_send();