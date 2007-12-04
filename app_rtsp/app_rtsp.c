/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Sergio Garcia Murillo <sergio.garcia@fontventa.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief MP4 application -- save and play mp4 files
 * 
 * \ingroup applications
 */

#include <asterisk.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <asterisk/lock.h>
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>
#include <asterisk/module.h>

#ifndef AST_FORMAT_AMRNB
#define AST_FORMAT_AMRNB	(1 << 13)
#endif 
#ifndef AST_FORMAT_MPEG4
#define AST_FORMAT_MPEG4        (1 << 22)
#endif

static char *name_rtsp = "rtsp";
static char *syn_rtsp = "rtsp player";
static char *des_rtsp = "  rtsp(url):  Play url. \n";

#define RTSP_NONE		0
#define RTSP_DESCRIBE		1
#define RTSP_SETUP_AUDIO 	2
#define RTSP_SETUP_VIDEO 	3
#define RTSP_PLAY 		4
#define RTSP_RELEASED 		5

static struct 
{
        int format;
        char* name;
} mimeTypes[] = {
	{ AST_FORMAT_G723_1, "G723"},
	{ AST_FORMAT_GSM, "GSM"},
	{ AST_FORMAT_ULAW, "PCMU"},
	{ AST_FORMAT_ALAW, "PCMA"},
	{ AST_FORMAT_G726, "G726-32"},
	{ AST_FORMAT_ADPCM, "DVI4"},
	{ AST_FORMAT_SLINEAR, "L16"},
	{ AST_FORMAT_LPC10, "LPC"},
	{ AST_FORMAT_G729A, "G729"},
	{ AST_FORMAT_SPEEX, "speex"},
	{ AST_FORMAT_ILBC, "iLBC"},
	{ AST_FORMAT_G722, "G722"},
	{ AST_FORMAT_G726_AAL2, "AAL2-G726-32"},
	{ AST_FORMAT_AMRNB, "AMR"},
	{ AST_FORMAT_JPEG, "JPEG"},
	{ AST_FORMAT_PNG, "PNG"},
	{ AST_FORMAT_H261, "H261"},
	{ AST_FORMAT_H263, "H263"},
	{ AST_FORMAT_H263_PLUS, "H263-1998"},
	{ AST_FORMAT_H263_PLUS, "H263-2000"},
	{ AST_FORMAT_H264, "H264"},
	{ AST_FORMAT_MPEG4, "MP4V-ES"},
};

typedef enum 
{
	RTCP_SR   = 200,
	RTCP_RR   = 201,
	RTCP_SDES = 202,
	RTCP_BYE  = 203,
	RTCP_APP  = 204
} RtcpType;

typedef enum 
{
	RTCP_SDES_END    =  0,
	RTCP_SDES_CNAME  =  1,
	RTCP_SDES_NAME   =  2,
	RTCP_SDES_EMAIL  =  3,
	RTCP_SDES_PHONE  =  4,
	RTCP_SDES_LOC    =  5,
	RTCP_SDES_TOOL   =  6,
	RTCP_SDES_NOTE   =  7,
	RTCP_SDES_PRIV   =  8,
	RTCP_SDES_IMG    =  9,
	RTCP_SDES_DOOR   = 10,
	RTCP_SDES_SOURCE = 11
} RtcpSdesType;


struct RtcpCommonHeader
{
	unsigned short version:2;  /* protocol version */
	unsigned short p:1;        /* padding flag */
	unsigned short count:5;    /* varies by payload type */
	unsigned short pt:8;       /* payload type */
	unsigned short length;   /* packet length in words, without this word */
};

struct RtcpReceptionReport
{
	unsigned int  ssrc;            /* data source being reported */
	unsigned int fraction:8;       /* fraction lost since last SR/RR */
	int lost:24;                   /* cumulative number of packets lost (signed!) */
	unsigned int  last_seq;        /* extended last sequence number received */
	unsigned int  jitter;          /* interarrival jitter */
	unsigned int  lsr;             /* last SR packet from this source */
	unsigned int  dlsr;            /* delay since last SR packet */
};

struct RtcpSdesItem
{
	unsigned char type;             /* type of SDES item (rtcp_sdes_type_t) */
	unsigned char length;           /* length of SDES item (in octets) */
	char data[1];                   /* text, not zero-terminated */
};

struct Rtcp
{
	struct RtcpCommonHeader common;    /* common header */
	union 
	{
		/* sender report (SR) */
		struct
		{
			unsigned int ssrc;        /* source this RTCP packet refers to */
			unsigned int ntp_sec;     /* NTP timestamp */
			unsigned int ntp_frac;
			unsigned int rtp_ts;      /* RTP timestamp */
			unsigned int psent;       /* packets sent */
			unsigned int osent;       /* octets sent */ 
			/* variable-length list */
			struct RtcpReceptionReport rr[1];
		} sr;

		/* reception report (RR) */
		struct 
		{
			unsigned int ssrc;        /* source this generating this report */
			/* variable-length list */
			struct RtcpReceptionReport rr[1];
		} rr;

		/* BYE */
		struct 
		{
			unsigned int src[1];      /* list of sources */
			/* can't express trailing text */
		} bye;

		/* source description (SDES) */
		struct rtcp_sdes_t 
		{
			unsigned int src;              /* first SSRC/CSRC */
			struct RtcpSdesItem item[1]; /* list of SDES items */
		} sdes;
	} r;
};

struct RtpHeader
{
    unsigned int cc:4;        /* CSRC count */
    unsigned int x:1;         /* header extension flag */
    unsigned int p:1;         /* padding flag */
    unsigned int version:2;   /* protocol version */
    unsigned int pt:7;        /* payload type */
    unsigned int m:1;         /* marker bit */
    unsigned int seq:16;      /* sequence number */
    unsigned int ts;          /* timestamp */
    unsigned int ssrc;        /* synchronization source */
    unsigned int csrc[1];     /* optional CSRC list */
};

struct RtspPlayer
{
	int	fd;
	int	state;
	int	cseq;
	char*	session[2];
	int 	numSessions;
	int 	end;

	char*	ip;
	int 	port;
	char*	url;

	int	audioRtp;
	int	audioRtcp;
	int	videoRtp;
	int	videoRtcp;

	int	audioRtpPort;
	int	audioRtcpPort;
	int	videoRtpPort;
	int	videoRtcpPort;
};

static struct RtspPlayer* RtspPlayerCreate(void)
{
	/* malloc */
	struct RtspPlayer* player = (struct RtspPlayer*) malloc(sizeof(struct RtspPlayer));
	/* Initialize */
	player->cseq 		= 1;
	player->session[0]	= NULL;
	player->session[1]	= NULL;
	player->numSessions	= 0;
	player->state 		= RTSP_NONE;
	player->end		= 0;
	player->ip		= NULL;
	player->port		= 0;
	player->url		= NULL;
	/* UDP */		
	player->fd		= 0;
	player->audioRtp	= 0;
	player->audioRtcp	= 0;
	player->videoRtp	= 0;
	player->videoRtcp	= 0;
	player->audioRtpPort	= 0;
	player->audioRtcpPort	= 0;
	player->videoRtpPort	= 0;
	player->videoRtcpPort	= 0;

	/* Return */
	return player;
}

static void RtspPlayerDestroy(struct RtspPlayer* player)
{
	/* free members*/
	if (player->ip) 	free(player->ip);
	if (player->url) 	free(player->url);
	if (player->session[0])	free(player->session[0]);
	if (player->session[1])	free(player->session[1]);
	/* free */
	free(player);
}

static void GetUdpPorts(int *a,int *b,int *p,int *q)
{
	struct sockaddr_in sendAddr;
	int len;

	/* empty addres */
	memset(&sendAddr,0,sizeof(struct sockaddr_in));

	sendAddr.sin_family = AF_INET;

	/* Create sockets */
	*a = socket(PF_INET,SOCK_DGRAM,0);
	bind(*a,(struct sockaddr *)&sendAddr,sizeof(struct sockaddr_in));
	*b = socket(PF_INET,SOCK_DGRAM,0);
	bind(*b,(struct sockaddr *)&sendAddr,sizeof(struct sockaddr_in));

	/* Get socket ports */
	len = sizeof(struct sockaddr_in);
	getsockname(*a,(struct sockaddr *)&sendAddr,&len);
	*p = ntohs(sendAddr.sin_port);
	len = sizeof(struct sockaddr_in);
	getsockname(*b,(struct sockaddr *)&sendAddr,&len);
	*q = ntohs(sendAddr.sin_port);

	ast_log(LOG_DEBUG,"-GetUdpPorts [%d,%d]\n",*p,*q);

	/* Create audio sockets */
	while ( *p%2 || *p+1!=*q )
	{
		/* Close first */
		close(*a);
		/* Move one forward */
		*a = *b;
		*p = *q;
		/* Create new socket */
		*b = socket(PF_INET,SOCK_DGRAM,0); 
		/* Get port */
		sendAddr.sin_port = htons(0);
		bind(*b,(struct sockaddr *)&sendAddr,sizeof(struct sockaddr_in));
		len = sizeof(struct sockaddr_in);
		getsockname(*b,(struct sockaddr *)&sendAddr,&len);
		*q = ntohs(sendAddr.sin_port);

		ast_log(LOG_DEBUG,"-GetUdpPorts [%d,%d]\n",*p,*q);
	}
}
static void SetNonBlocking(int fd)
{
	/* Get flags */
	int flags = fcntl(fd,F_GETFD);

	/* Set socket non-blocking */
	fcntl(fd,F_SETFD,flags | O_NONBLOCK);
}

static int RtspPlayerConnect(struct RtspPlayer *player, const char *ip, int port)
{
	struct sockaddr_in sendAddr;

	/* open socket */
	player->fd = socket(PF_INET,SOCK_STREAM,0);

	/* Open audio ports */
	GetUdpPorts(&player->audioRtp,&player->audioRtcp,&player->audioRtpPort,&player->audioRtcpPort);

	/* Open video ports */
	GetUdpPorts(&player->videoRtp,&player->videoRtcp,&player->videoRtpPort,&player->videoRtcpPort);

	/* Set non blocking */
	SetNonBlocking(player->fd);
	SetNonBlocking(player->audioRtp);
	SetNonBlocking(player->audioRtcp);
	SetNonBlocking(player->videoRtp);
	SetNonBlocking(player->videoRtcp);

	/* empty addres */
	memset(&sendAddr,0,sizeof(struct sockaddr_in));

	/* Set data */
	sendAddr.sin_family	 = AF_INET;
	sendAddr.sin_addr.s_addr = INADDR_ANY;
	sendAddr.sin_addr.s_addr = inet_addr(ip);
	sendAddr.sin_port	 = htons(port);

	/* Connect */
	if (connect(player->fd,(struct sockaddr *)&sendAddr,sizeof(sendAddr)))
		/* Exit */
		return 0;

	/* copy ip & port*/
	player->ip 	= strdup(ip);
	player->port 	= port;

	/* conected */
	return 1;
}
static int RtspPlayerAddSession(struct RtspPlayer *player,char *session)
{
	int i;

	/* If max sessions reached */
	if (player->numSessions == 2)
		/* Exit */
		return 0;

	/* Check if we have that session alreadY */
	for (i=0;i<player->numSessions;i++)
		if (strcmp(player->session[i],session)==0)
		{
			/* Free */
			free(session);
			/* exit */
			return 0;
		}
	/* Save */
	player->session[player->numSessions++] = session;

	/* exit */
	return player->numSessions;
	

}
static void RtspPlayerClose(struct RtspPlayer *player)
{
	/* Close sockets */
	if (player->fd)		close(player->fd);
	if (player->audioRtp)	close(player->audioRtp);
	if (player->audioRtcp)	close(player->audioRtcp);
	if (player->videoRtp)	close(player->videoRtp);
	if (player->videoRtcp)	close(player->videoRtcp);
}

static int SendRequest(int fd,char *request,int *end)
{
	/* Get request len */
	int len = strlen(request);
	/* Send request */
	if (send(fd,request,len,0)==-1)
	{
		/* If failed connection*/
		if (errno!=EAGAIN)
		{
			/* log */
			ast_log(LOG_ERROR,"Error sending request [%d]\n",errno);
			/* End */
			*end = 0;
		}
		/* exit*/
		return 0;
	}
	/* Return length */
	return len;
}

static int RtspPlayerDescribe(struct RtspPlayer *player,const char *url)
{

	char request[1024];

	/* Log */
	ast_log(LOG_DEBUG,">DESCRIBE [%s]\n",url);

	/* Prepare request */
	snprintf(request,1024,
			"DESCRIBE rtsp://%s%s RTSP/1.0\r\n"
			"CSeq: %d\r\n"
			"Accept: application/sdp\r\n"
			"User-Agent: app_rtsp\r\n"
			"\r\n",
			player->ip,url,player->cseq);

	/* Send request */
	if (!SendRequest(player->fd,request,&player->end))
		/* exit */
		return 0;
	/* Save url */
	player->url = strdup(url);
	/* Set state */
	player->state = RTSP_DESCRIBE;
	/* Increase seq */
	player->cseq++;
	ast_log(LOG_DEBUG,"<DESCRIBE [%s]\n",url);
	/* ok */
	return 1;
}

static int RtspPlayerSetupAudio(struct RtspPlayer* player,const char *url)
{
	char request[1024];
	char sessionheader[256];

	/* Log */
	ast_log(LOG_DEBUG,"-SETUP AUDIO [%s]\n",url);

	/* if it got session */
	if (player->numSessions)
		/* Create header */
		snprintf(sessionheader,256,"Session: %s\r\n",player->session[player->numSessions-1]);
	else
		/* no header */
		sessionheader[0] = 0;

	/* If it's absolute */
	if (strncmp(url,"rtsp://",7)==0)
	{
		/* Prepare request */
		snprintf(request,1024,
				"SETUP %s RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"%s"
				"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
				"User-Agent: app_rtsp\r\n"
				"\r\n",
				url,player->cseq,sessionheader,player->audioRtpPort,player->audioRtcpPort);
	} else {
		/* Prepare request */
		snprintf(request,1024,
				"SETUP rtsp://%s%s/%s RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"%s"
				"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
				"User-Agent: app_rtsp\r\n"
				"\r\n",
				player->ip,player->url,url,player->cseq,sessionheader,player->audioRtpPort,player->audioRtcpPort);
	}

	/* Send request */
	if (!SendRequest(player->fd,request,&player->end))
		/* exit */
		return 0;
	/* Set state */
	player->state = RTSP_SETUP_AUDIO;
	/* Increase seq */
	player->cseq++;
	/* ok */
	return 1;
}

static int RtspPlayerSetupVideo(struct RtspPlayer* player,const char *url)
{
	char request[1024];
	char sessionheader[256];

	/* Log */
	ast_log(LOG_DEBUG,"-SETUP VIDEO [%s]\n",url);

	/* if it got session */
	if (player->numSessions)
		/* Create header */
		snprintf(sessionheader,256,"Session: %s\r\n",player->session[player->numSessions-1]);
	else
		/* no header */
		sessionheader[0] = 0;

	/* If it's absolute */
	if (strncmp(url,"rtsp://",7)==0)
	{
		/* Prepare request */
		snprintf(request,1024,
				"SETUP %s RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"%s"
				"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
				"User-Agent: app_rtsp\r\n"
				"\r\n",
				url,player->cseq,sessionheader,player->videoRtpPort,player->videoRtcpPort);
	} else {
		/* Prepare request */
		snprintf(request,1024,
				"SETUP rtsp://%s%s/%s RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"%s"
				"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n"
				"User-Agent: app_rtsp\r\n"
				"\r\n",
				player->ip,player->url,url,player->cseq,sessionheader,player->videoRtpPort,player->videoRtcpPort);
	}

	/* Send request */
	if (!SendRequest(player->fd,request,&player->end))
		/* exit */
		return 0;
	/* Set state */
	player->state = RTSP_SETUP_VIDEO;
	/* Increase seq */
	player->cseq++;
	/* ok */
	return 1;
}

static int RtspPlayerPlay(struct RtspPlayer* player)
{
	char request[1024];
	int i;

	/* Log */
	ast_log(LOG_DEBUG,"-PLAY [%s]\n",player->url);

	/* if not session */
	if (!player->numSessions)
		/* exit*/
		return 0;

	/* For each request pipeline */
	for (i=0;i<player->numSessions;i++)
	{
		/* Prepare request */
		snprintf(request,1024,
				"PLAY rtsp://%s%s RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"Session: %s\r\n"
				"User-Agent: app_rtsp\r\n"
				"\r\n",
				player->ip,player->url,player->cseq,player->session[i]);

		/* Send request */
		if (!SendRequest(player->fd,request,&player->end))
			/* exit */
			return 0;
		/* Increase seq */
		player->cseq++;
	}
	/* Set state */
	player->state = RTSP_PLAY;
	/* ok */
	return 1;
}

static int RtspPlayerTeardown(struct RtspPlayer* player)
{
	char request[1024];
	int i;

	/* Log */
	ast_log(LOG_DEBUG,"-TEARDOWN\n");

	/* if not session */
	if (!player->numSessions)
		/* exit*/
		return 0;

	/* For each request pipeline */
	for (i=0;i<player->numSessions;i++)
	{
		/* Prepare request */
		snprintf(request,1024,
				"TEARDOWN rtsp://%s%s RTSP/1.0\r\n"
				"CSeq: %d\r\n"
				"Session: %s\r\n"
				"User-Agent: app_rtsp\r\n"
				"\r\n",
				player->ip,player->url,player->cseq,player->session[i]);
		/* Send request */
		if (!SendRequest(player->fd,request,&player->end))
			/* exit */
			return 0;
		/* Increase seq */
		player->cseq++;
	}
	/* Set state */
	player->state = RTSP_RELEASED;
	/* ok */
	return 1;
}

#define RTSP_TUNNEL_CONNECTING 	0
#define RTSP_TUNNEL_NEGOTIATION 1
#define RTSP_TUNNEL_RTP 	2


struct SDPFormat
{
	int 	payload;
	int	format;	
	char*	control;
};

struct SDPMedia
{
	struct SDPFormat** formats;
	int num;
};

struct SDPContent
{
	struct SDPMedia* audio;
	struct SDPMedia* video;
};

static struct SDPMedia* CreateMedia(char *buffer,int bufferLen)
{
	int num = 0;
	int i = 0;
	struct SDPMedia* media = NULL;;

	/* Count number of spaces*/
	for (i=0;i<bufferLen;i++)
		/* If it's a withespace */
		if (buffer[i]==' ')
			/* Anopther one */
			num++;

	/* if no media */
	if (num<3)
		/* Exit */
		return NULL;

	/* Allocate */
	media = (struct SDPMedia*) malloc(sizeof(struct SDPMedia));

	/* Get number of formats */
	media->num = num - 2; 

	/* Allocate */
	media->formats = (struct SDPFormat**) malloc(media->num);

	/* For each format */
	for (i=0;i<media->num;i++)
	{
		/* Allocate format */
		media->formats[i] = (struct SDPFormat*) malloc(media->num);
		/* Init params */
		media->formats[i]->payload 	= -1;
		media->formats[i]->format 	= 0;
		media->formats[i]->control 	= NULL;
	}

	/* log */
	ast_log(LOG_DEBUG,"-creating media [%d,%s]\n",media->num,strndup(buffer,bufferLen));

	/* Return media */
	return media;
}

static void DestroyMedia(struct SDPMedia* media)
{
	int i = 0;

	/* Free format */
	for (i=0;i<media->num;i++)
	{
		/* Free control */
		if (media->formats[i]->control) 
			free(media->formats[i]->control);
		/* Free format*/
		free(media->formats[i]);
	}
	/* Free format */
	free(media->formats);
	/* Free media */
	free(media);
}
static struct SDPContent* CreateSDP(char *buffer,int bufferLen)
{
	struct SDPContent* sdp = NULL;
	struct SDPMedia* media = NULL;;
	char *i = buffer;
	char *j = NULL;
	char *ini;
	char *end;
	int n = 0;
	int f = 0;

	/* Malloc */
	sdp = (struct SDPContent*) malloc(sizeof(struct SDPContent));

	/* NO audio and video */
	sdp->audio = NULL;
	sdp->video = NULL;

	/* Read each line */
	while ( (j=strstr(i,"\n")) != NULL)
	{
		/* if it¡s not enougth data */
		if (j-i<=1)
			goto next;

		/* If previous is a \r" */
		if (j[-1]=='\r')
			/* Decrease end */
			j--;

		/* log */
		ast_log(LOG_DEBUG,"-line [%s]\n",strndup(i,j-i));

		/* Check header */
		if (strncmp(i,"m=",2)==0) 
		{
			/* media */
			if (strncmp(i+2,"video",5)==0)
			{
				/* create video */
				sdp->video = CreateMedia(i,j-i);
				/* set current media */
				media = sdp->video;
			} else if (strncmp(i+2,"audio",5)==0) {
				/* create video */
				sdp->audio = CreateMedia(i,j-i);
				/* set current media */
				media = sdp->audio;
			} else 
				/* no media */
				media = NULL;
			/* reset formats */
			n = 0;
		} else if (strncmp(i,"a=rtpmap:",9)==0){
			/* if not in media */
			if (!media)
				goto next;
			/* If more than formats */
			if (n==media->num)
				goto next;
			/* get ini */
			for (ini=i;ini<j;ini++)
				/* if it¡s space */
				if (*ini==' ')
					break;
			/* skip space*/
			if (++ini>=j)
				goto next;
			/* get end */
			for (end=ini;end<j;end++)
				/* if it¡s space */
				if (*end=='/')
					break;
			/* Check formats */
			for (f = 0; f < sizeof(mimeTypes)/sizeof(mimeTypes[0]); ++f) 
				/* If the string is in it */
				if (strncasecmp(ini,mimeTypes[f].name,end-ini) == 0) 
				{
					/* Set type */
					media->formats[n]->format = mimeTypes[f].format;
					/* Set payload */
					media->formats[n]->payload = atoi(i+9);
					/* Exit */
					break;
				}
			/* Inc medias */
			n++;
			
		} else if (strncmp(i,"a=control:",10)==0){
			/* if not in media */
			if (!media)
				goto next;
			/* If more than formats */
			if (n>media->num)
				goto next;
			/* If it's previous to the ftmp */
			if (n==0)
			{
				/* Set control for all */
				for ( f=0; f<media->num; f++)
					/* Set */
					media->formats[f]->control = strndup(i+10,j-i-10);
			} else  {
				/* if already had control */
				if (media->formats[n-1]->control)
					/* Free it */
					free(media->formats[n-1]->control);
				/* Get new control */
			 	media->formats[n-1]->control = strndup(i+10,j-i-10);
			}
		}
next:
		/* if it's a \r */
		if (j[0]=='\r')
			/* skip \r\n to next line */
			i = j+2;
		else
			/* skip \n to next line */
			i = j+1;
	}

	/* Return sdp */
	return sdp;
}

static void DestroySDP(struct SDPContent* sdp)
{
	/* Free medias */
	if (sdp->audio) DestroyMedia(sdp->audio);
	if (sdp->video) DestroyMedia(sdp->video);
	/* Free */
	free(sdp);
}


static int HasHeader(char *buffer,int bufferLen,char *header)
{
	int len;
	char *i;

	/* Get length */
	len = strlen(header);

	/* If no header*/
	if (!len)
		/* Exit */
		return 0;

	/* Get Header */
	i = strcasestr(buffer,header);

	/* If not found or not \r\n first*/
	if (i<buffer+2)
		/* Exit */
		return 0;

	/* If it's not in this request */
	if (i-buffer>bufferLen)
		/* Exit */
		return 0;

	/* Check for \r\n */
	if (i[-2]!='\r' || i[-1]!='\n')
		/* Exit */
		return 0;

	/* Check for ": " */
	if (i[len]!=':' || i[len+1]!=' ')
		/* Exit */
		return 0;

	/* Return value start */
	return (i-buffer)+len+2;
}

static int GetHeaderValueInt(char *buffer,int bufferLen,char *header)
{
	int i;	 

	/* Get start */
	if (!(i=HasHeader(buffer,bufferLen,header)))
		/* Exit */
		return 0;

	/* Return value */
	return atoi(buffer+i);
}

static long GetHeaderValueLong(char *buffer,int bufferLen,char *header)
{
	int i;	 

	/* Get start */
	if (!(i=HasHeader(buffer,bufferLen,header)))
		/* Exit */
		return 0;

	/* Return value */
	return atol(buffer+i);
}

static char* GetHeaderValue(char *buffer,int bufferLen,char *header)
{
	int i;	 
	char *j;

	/* Get start */
	if (!(i=HasHeader(buffer,bufferLen,header)))
		/* Exit */
		return 0;
	/* Get end */
	if (!(j=strstr(buffer+i,"\r\n")))
		/* Exit */
		return 0;

	/* Return value */
	return strndup(buffer+i,(j-buffer)-i);
}

static int CheckHeaderValue(char *buffer,int bufferLen,char *header,char*value)
{
	int i;	 

	/* Get start */
	if (!(i=HasHeader(buffer,bufferLen,header)))
		/* Exit */
		return 0;

	/* Return value */
	return (strncasecmp(buffer+i,value,strlen(value))==0);
}



static int RecvResponse(int fd,char *buffer,int *bufferLen,int bufferSize,int *end)
{
	/* Read into buffer */
	int len = recv(fd,buffer,bufferSize-*bufferLen,0);

	/* if error or closed */
	if (!len>0)
	{
		/* If failed connection*/
		if ((errno!=EAGAIN && errno!=EWOULDBLOCK) || !len)
		{
			/* log */
			ast_log(LOG_ERROR,"Error receiving response [%d,%d]\n",len,errno);
			/* End */
			*end = 1;
		}
		/* exit*/
		return 0;
	} 
	/* Increase buffer length */
	*bufferLen += len;
	/* Finalize as string */
	buffer[*bufferLen] = 0;
	/* Return len */
	return len;
}

static int GetResponseLen(char *buffer)
{
	char *i;
	/* Search end of response */
	if ((i=strstr(buffer,"\r\n\r\n"))==NULL)
		/*Exit*/
		return 0;
	/* Get msg leng */
	return i-buffer+4;
}


static int rtsp_play(struct ast_channel *chan,char *ip, int port, char *url)
{
	struct ast_frame *f = NULL;
	struct ast_frame *sendFrame = NULL;

	int infds[5];
	int outfd;

	char buffer[16384];
	int  bufferSize = 16383; /* One less for finall \0 */
	int  bufferLen = 0;
	int  responseLen = 0;
	int  contentLength = 0;
	char *rtpBuffer;
	char rtcpBuffer[1500];
	int  rtpSize = 1500;
	int  rtcpSize = 1500;
	int  rtpLen = 0;
	int  rtcpLen = 0;
	char *session;
	char *range;
	char *j;
	char src[128];
	int  res = 0;

	struct SDPContent* sdp = NULL;
	char *audioControl = NULL;
	char *videoControl = NULL;
	int audioFormat = 0;
	int videoFormat = 0;
	int audioType = 0;
	int videoType = 0;
	unsigned int lastVideo = 0;
	unsigned int lastAudio = 0;

	int duration = 0;
	int elapsed = 0;
	int ms = 10000;
	int i = 0;

	struct RtspPlayer *player;
	struct RtpHeader *rtp;
	struct Rtcp *rtcp;
	struct timeval tv = {0,0};

	/* log */
	ast_log(LOG_WARNING,">rtsp play\n");

	/* Set random src */
	sprintf(src,"rtsp_play%08lx", ast_random());

	/* Create player */
	player = RtspPlayerCreate();

	/* if error */
	if (!player)
	{
		/* log */
		ast_log(LOG_ERROR,"Couldn't create player\n");
		/* exit */
		return 0;
	}

	/* Connect player */
	if (!RtspPlayerConnect(player,ip,port))
	{
		/* log */
		ast_log(LOG_ERROR,"Couldn't connect to %s:%d\n",ip,port);
		/* end */
		goto rtsp_play_clean;
	}

	/* Set arrays */
	infds[0] = player->fd;
	infds[1] = player->audioRtp;
	infds[2] = player->videoRtp;
	infds[3] = player->audioRtcp;
	infds[4] = player->videoRtcp;

	/* Send request */
	if (!RtspPlayerDescribe(player,url))
	{
		/* log */
		ast_log(LOG_ERROR,"Couldn't handle DESCRIBE in %s\n",url);
		/* end */
		goto rtsp_play_end;
	}

	/* malloc frame & data */
	sendFrame = (struct ast_frame *) malloc(sizeof(struct ast_frame) + rtpSize);

	/* Set data pointer */
	rtpBuffer = (void*)sendFrame + AST_FRIENDLY_OFFSET;

	/* log */
	ast_log(LOG_DEBUG,"-rtsp play loop\n");

	/* Loop */
	while(!player->end)
	{
		/* No output */
		outfd = -1;
		/* If the playback has started */
		if (!ast_tvzero(tv))
		{
			/* Get playback time */
			elapsed = ast_tvdiff_ms(ast_tvnow(),tv); 
			/* Check how much time have we been playing */
			if (elapsed>=duration)
			{
				/* log */
				ast_log(LOG_DEBUG,"Playback finished\n");
				/* exit */
				player->end = 1;
				/* Exit */
				break;
			} else {
				/* Set timeout to remaining time*/
				ms = duration-elapsed;
			}
		} else {
			/* 4 seconds timeout */
			ms = 4000;
		}

		/* Read from channels and fd*/
		if (ast_waitfor_nandfds(&chan,1,infds,5,NULL,&outfd,&ms))
		{
			/* Read frame */
			f = ast_read(chan);

			/* If failed */
			if (!f) 
				/* exit */
				break;
			
			/* If it's a control channel */
			if (f->frametype == AST_FRAME_CONTROL) 
			{
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP)
				{
					/* log */
					ast_log(LOG_DEBUG,"-Hangup\n");
					/* exit */
					player->end = 1;
				}
				
			 /* If it's a dtmf */
                        } else if (f->frametype == AST_FRAME_DTMF) {
				char dtmf[2];
				/* Get dtmf number */
				dtmf[0] = f->subclass;
				dtmf[1] = 0;

				/* Check for dtmf extension in context */
				if (ast_exists_extension(chan, chan->context, dtmf, 1, NULL)) {
					/* Set extension to jump */
					res = atoi(dtmf);
					/* Free frame */
					ast_frfree(f);
					/* exit */
					goto rstp_play_stop;
				}
			}

			/* free frame */
			ast_frfree(f);
		} else if (outfd==player->fd) {
			/* Depending on state */	
			switch (player->state)
			{
				case RTSP_DESCRIBE:
					/* log */
					ast_log(LOG_DEBUG,"-Receiving describe\n");
					/* Read into buffer */
					if (!RecvResponse(player->fd,buffer,&bufferLen,bufferSize,&player->end))
						break;
					/* If not reading content */
					if (contentLength==0)
					{
						/* Search end of response */
						if ( (responseLen=GetResponseLen(buffer)) == 0 )
							/*Exit*/
							break;

						/* Does it have content */
						contentLength = GetHeaderValueInt(buffer,responseLen,"Content-Length");	
						/* Is it sdp */
						if (!CheckHeaderValue(buffer,responseLen,"Content-Type","application/sdp"))
						{
							/* log */
							ast_log(LOG_ERROR,"Content-Type unknown\n");
							/* End */
							player->end = 1;
						}
						/* Get new length */
						bufferLen -= responseLen;
						/* Move data to begining */
						memcpy(buffer,buffer+responseLen,bufferLen);
					}
					
					/* If there is not enough data */	
					if (bufferLen<contentLength) 
						/* break */
						break;

					/* Create SDP */
					sdp = CreateSDP(buffer,contentLength);
					/* Get new length */
					bufferLen -= contentLength;
					/* Move data to begining */
					memcpy(buffer,buffer+responseLen,bufferLen);
					/* Reset content */
					contentLength = 0;

					/* If not sdp */
					if (!sdp)
					{
						/* log */
						ast_log(LOG_ERROR,"Couldn't parse SDP\n");
						/* end */
						player->end = 1;
						/* exit */
						break;
					}

					/* Get best audio track */
					if (sdp->audio)
						/* Get first matching format */
						for (i=0;i<sdp->audio->num;i++)
						{
							/* log */
							ast_log(LOG_DEBUG,"-audio [%d,%d,%s]\n", sdp->audio->formats[i]->format, sdp->audio->formats[i]->payload ,sdp->audio->formats[i]->control);
							/* if we have that */
							if (sdp->audio->formats[i]->format & chan->nativeformats)
							{
								/* Store type */
								audioType = sdp->audio->formats[i]->payload;
								/* Store format */
								audioFormat = sdp->audio->formats[i]->format;
								/* Store control */
								audioControl = sdp->audio->formats[i]->control;
								/* Got a valid one */
								break;
							}
						}

					/* Get best video track */
					if (sdp->video)
						/* Get first matching format */
						for (i=0;i<sdp->video->num;i++)
						{
							/* log */
							ast_log(LOG_DEBUG,"-video [%d,%d,%s]\n", sdp->video->formats[i]->format, sdp->video->formats[i]->payload ,sdp->video->formats[i]->control);
							/* if we have that */
							if (sdp->video->formats[i]->format & chan->nativeformats)
							{
								/* Store type */
								videoType = sdp->video->formats[i]->payload;
								/* Store format */
								videoFormat = sdp->video->formats[i]->format;
								/* Store control */
								videoControl = sdp->video->formats[i]->control;
								/* Got a valid one */
								break;
							}
						}

					/* Set write format */
					ast_set_write_format(chan, audioFormat | videoFormat);	

					/* if audio track */
					if (audioControl)
					{
						/* Open audio */
						RtspPlayerSetupAudio(player,audioControl);
					} else if (videoControl) {
						/* Open video */
						RtspPlayerSetupVideo(player,videoControl);
					} else {
						/* log */
						ast_log(LOG_ERROR,"No media found\n");
						/* end */
						player->end = 1;
					}
					break;

				case RTSP_SETUP_AUDIO:
					/* log */
					ast_log(LOG_DEBUG,"-Recv audio response\n");
					/* Read into buffer */
					if (!RecvResponse(player->fd,buffer,&bufferLen,bufferSize,&player->end))
						break;
					/* Search end of response */
					if ( (responseLen=GetResponseLen(buffer)) == 0 )
						/*Exit*/
						break;

					/* Does it have content */
					if (GetHeaderValueInt(buffer,responseLen,"Content-Length"))
					{
						/* log */
						ast_log(LOG_ERROR,"Content length not expected\n");
						/* Uh? */
						player->end = 1;
						/* break */
						break;
					}
					/* Get session */
					if ( (session=GetHeaderValue(buffer,responseLen,"Session")) == 0)
					{
						/* log */
						ast_log(LOG_ERROR,"No session [%s]\n",buffer);
						/* Uh? */
						player->end = 1;
						/* break */
						break;
					}
					/* Append session to player */
					RtspPlayerAddSession(player,session);
					/* Get new length */
					bufferLen -= responseLen;
					/* Move data to begining */
					memcpy(buffer,buffer+responseLen,bufferLen);
					/* If video control */
					if (videoControl)
						/* Set up video */
						RtspPlayerSetupVideo(player,videoControl);
					else 
						/* play */
						RtspPlayerPlay(player);
					break;
				case RTSP_SETUP_VIDEO:
					/* Read into buffer */
					if (!RecvResponse(player->fd,buffer,&bufferLen,bufferSize,&player->end))
						break;
					/* Search end of response */
					if ( (responseLen=GetResponseLen(buffer)) == 0 )
						/*Exit*/
						break;

					/* Does it have content */
					if (GetHeaderValueInt(buffer,responseLen,"Content-Length"))
					{
						/* log */
						ast_log(LOG_ERROR,"No content length\n");
						/* Uh? */
						player->end = 1;
						/* break */
						break;
					}
					/* Get session if we don't have already one*/
					if ( (session=GetHeaderValue(buffer,responseLen,"Session")) == 0)
					{
						/* log */
						ast_log(LOG_ERROR,"No session [%s]\n",buffer);
						/* Uh? */
						player->end = 1;
						/* break */
						break;
					}
					/* Append session to player */
					RtspPlayerAddSession(player,session);
					/* Get new length */
					bufferLen -= responseLen;
					/* Move data to begining */
					memcpy(buffer,buffer+responseLen,bufferLen);
					/* Play */
					RtspPlayerPlay(player);
					break;
				case RTSP_PLAY:
					/* Read into buffer */
					if (!RecvResponse(player->fd,buffer,&bufferLen,bufferSize,&player->end))
						break;
					/* Search end of response */
					if ( (responseLen=GetResponseLen(buffer)) == 0 )
						/*Exit*/
						break;
					/* Get range */
					if ( (range=GetHeaderValue(buffer,responseLen,"Range")) == 0)
					{
						/* No end of stream */
						duration = -1;
					} else {
						/* Get end part */
						j = strchr(range,'-');
						/* Check format */
						if (j)
							/* Get duration */
							duration = atof(j+1)*1000;  
						else 
							/* No end of stream */
							duration = -1;
						/* Free string */
						free(range);
					}
					/* If the video has end */
					if (duration!=-1)
						/* Init counter */
						tv = ast_tvnow();
					/* log */
					ast_log(LOG_DEBUG,"-Started playback [%d]\n",duration);
					/* Get new length */
					bufferLen -= responseLen;
					/* Move data to begining */
					memcpy(buffer,buffer+responseLen,bufferLen);
					break;
			}
		} else if ((outfd==player->audioRtp) ||  (outfd==player->videoRtp) ) {
			/* Set length */
			rtpLen = 0;
			
			/* Clean frame */
			memset(sendFrame,0,sizeof(struct ast_frame) + rtpSize);


			/* Read rtp packet */
			if (!RecvResponse(outfd,rtpBuffer,&rtpLen,rtpSize,&player->end))
				break;

			/* If not got enought data */
			if (rtpLen<12)
				/*exit*/
				break;

			/* Get headers */
			rtp = (struct RtpHeader*)rtpBuffer;

			/* Set data ini */
			int ini = sizeof(struct RtpHeader)-4;

			/* Increase length */
			ini += rtp->cc;

			/* Get timestamp */
			unsigned int ts = ntohl(rtp->ts);
			 
			/* Set frame data */
			sendFrame->data = rtpBuffer+ini;
			sendFrame->datalen = rtpLen-ini;
			sendFrame->src = strdup(src);

			/* Depending on socket */
			if (outfd==player->audioRtp) {
				/* Set type */
				sendFrame->frametype = AST_FRAME_VOICE;
				sendFrame->subclass =  audioFormat;
				/* Set number of samples */
				if (lastAudio)
					/* Set number of samples */
					sendFrame->samples = ts-lastAudio;
				else
					/* Set number of samples to 160 */
					sendFrame->samples = 160;
				/* Save ts */
				lastAudio = ts;
			} else {
				/* Set type */
				sendFrame->frametype = AST_FRAME_VIDEO;
				sendFrame->subclass = videoFormat;
				/* If not the first */
				if (lastVideo)
					/* Set number of samples */
					sendFrame->samples = ts-lastVideo;
				else
					/* Set number of samples to 0 */
					sendFrame->samples = 0;
				/* Save ts */
				lastVideo = ts;
				/* Set mark */
				sendFrame->subclass |= rtp->m;
			}

			/* Rest */
			sendFrame->delivery.tv_usec = 0;
			sendFrame->delivery.tv_sec = 0;
			/* Don't free the frame outside */
			sendFrame->mallocd = 0;
			/* Send frame */
			ast_write(chan,sendFrame);

		} else if ((outfd==player->audioRtcp) || (outfd==player->videoRtcp)) {
			/* Set length */
			rtcpLen = 0;
			i = 0;
			
			/* Read rtcp packet */
			if (!RecvResponse(outfd,rtcpBuffer,&rtcpLen,rtcpSize,&player->end))
				break;

			/* Process rtcp packets */
			while(i<rtcpLen)
			{
				/* Get packet */
				rtcp = (struct Rtcp*)(rtcpBuffer+i);
				/* Increase pointer */
				i += (ntohs(rtcp->common.length)+1)*4;
				/* Check for bye */
				if (rtcp->common.pt == RTCP_BYE)
				{
					/* End playback */
					player->end = 1;
					/* exit */	
					break;
				}
			}

		} else if (player->state!=RTSP_PLAY) {
			/* log */
			ast_log(LOG_ERROR,"-timedout and not conected [%d]",outfd);
			/* Exit f timedout and not conected*/
			player->end = 1;
		} 
	}

rstp_play_stop:

	/* log */
	ast_log(LOG_DEBUG,"-rtsp_play end loop [%d]\n",res);

	/* Send teardown if something was setup */
	if (player->state>RTSP_DESCRIBE)
		/* Teardown */
		RtspPlayerTeardown(player);

	/* Free frame */
	if (sendFrame)
		/* Free memory */
		free(sendFrame);

	/* If ther was a sdp */
	if (sdp)
		/* Destroy it */
		DestroySDP(sdp);

rtsp_play_clean:
	/* Close socket */
	RtspPlayerClose(player);

rtsp_play_end:
	/* Destroy player */
	RtspPlayerDestroy(player);

	/* log */
	ast_log(LOG_WARNING,"<rtsp_play");

	/* Exit */	
	return res;
}

static int rtsp_tunnel(struct ast_channel *chan,char *ip, int port, char *url)
{
	struct sockaddr_in sendAddr;
	struct ast_frame *f;

	int infds[1];
	int outfd;
	int rtsp;

	int state = RTSP_TUNNEL_CONNECTING;
	char request[1024];
	char buffer[16384];
	char *i;
	int  bufferSize = 16383; /* One less for finall \0 */
	int  bufferLen = 0;
	int  responseLen = 0;
	int  contentLength = 0;

	struct SDPContent* sdp;
	int  isSDP;

	int end = 0;
	int ms = 10000;
	int flags;


	/* open socket */
	rtsp = socket(PF_INET,SOCK_STREAM,0);

	/* empty addres */
	memset(&sendAddr,0,sizeof(struct sockaddr_in));

	/* Set data */
	sendAddr.sin_family	 = AF_INET;
	sendAddr.sin_addr.s_addr = INADDR_ANY;
	sendAddr.sin_addr.s_addr = inet_addr(ip);
	sendAddr.sin_port	 = htons(port);

	/* Get flags */
	flags = fcntl(rtsp,F_GETFD);

	/* Set socket non-blocking */
	fcntl(rtsp,F_SETFD,flags | O_NONBLOCK);

	/* Connect */
	if (connect(rtsp,(struct sockaddr *)&sendAddr,sizeof(sendAddr)))
		/* Exit */
		return 0;

	/* Prepare request */
	snprintf(request,1024,"GET %s HTTP/1.0\r\nUser-Agent: app_rtsp\r\n Accept: application/x-rtsp-tunnelled\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n\r\n",url);


	/* Set arrays */
	infds[0] = rtsp;

	/* Loop */
	while(!end)
	{
		/* No output */
		outfd = -1;
		/* Read from channels and fd*/
		if (ast_waitfor_nandfds(&chan,1,infds,1,NULL,&outfd,&ms))
		{
			/* Read frame */
			f = ast_read(chan);

			/* If failed */
			if (!f) 
				/* exit */
				break;
			
			/* If it's a control channel */
			if (f->frametype == AST_FRAME_CONTROL) 
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP)
					/* exit */
					end = 1;
			/* free frame */
			ast_frfree(f);
		} else if (outfd==rtsp) {
			/* Depending on state */	
			switch (state)
			{
				case RTSP_TUNNEL_CONNECTING:
					/* Send request */
					if (!SendRequest(rtsp,request,&end))
						/* exit*/
						break;
					/* It has been opened and sent*/
					state = RTSP_TUNNEL_NEGOTIATION;	
					break;
				case RTSP_TUNNEL_NEGOTIATION:
					/* Read into buffer */
					if (!RecvResponse(rtsp,buffer,&bufferLen,bufferSize,&end))
						break;
					/* Process */
					while (1)
					{	
						/* If not reading content */
						if (contentLength==0)
						{
							/* Search end of response */
							if ( (responseLen=GetResponseLen(buffer)) == 0 )
								/*Exit*/
								break;
							/* Does it have content */
							contentLength = GetHeaderValueInt(buffer,responseLen,"Content-Length");	
							/* Is it sdp */
							if (CheckHeaderValue(buffer,responseLen,"Content-Type","application/sdp"))
								/* SDP */
								isSDP = 1;
							else
								/* NO SDP*/
								isSDP = 0;
							/* If we have the sdp already */
							if (sdp && HasHeader(buffer,responseLen,"RTP-Info"))
								/* RTP */
								state = RTSP_TUNNEL_RTP;
							/* Get new length */
							bufferLen -= responseLen;
							/* Move data to begining */
							memcpy(buffer,buffer+responseLen,bufferLen);
			
						/* If there is enough data */	
						} else if (bufferLen>=contentLength) {
							/* If it's the sdp */
							if (isSDP)
								/* Create SDP */
								sdp = CreateSDP(buffer,contentLength);
							/* Get new length */
							bufferLen -= contentLength;
							/* Move data to begining */
							memcpy(buffer,i,bufferLen);
							/* Reset content */
							contentLength = 0;
						} else
							break;
					}
					break;
				case RTSP_TUNNEL_RTP:
					break;
			}
		} else if (state==RTSP_TUNNEL_CONNECTING) 
			/* Exit f timedout and not conected*/
			end = 1;
	}

	/* If ther was a sdp */
	if (sdp)
		/* Destroy it */
		DestroySDP(sdp);

	/* Close socket */
	close(rtsp);

	/* Exit */	
	return 0;
}

static int app_rtsp(struct ast_channel *chan, void *data)
{
	struct ast_module_user *u;
	char *uri;
	char *ip;
	char *url;
	char *i;
	int  port;

	/* Get data */
	uri = (char*)data;

	/* Get proto part */
	if ((i=strstr(uri,"://"))==NULL)
	{
		ast_log(LOG_ERROR,"RTSP ERROR: Invalid uri %s\n",uri);
		return 0;
	}

	/* Increase url */
	url = i + 3; 

	/* Get server part */
	if ((i=strstr(url,"/"))!=NULL)
	{
		/* Assign server */
		ip = strndup(url,i-url);
		/* Get url */
		url = i;
	} else {
		/* all is server */
		ip = strdup(url);
		/* Get root */	
		url = "/";
	}

	/* Get port */
	if ((i=strstr(ip,":"))!=NULL)
	{
		/* Get port */
		port = atoi(i+1);
		/* Remove from server */
		i[0] = 0;
	} else {
		/* No port */
		port = 0;
	}

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Depending on protocol */
	if (strncmp(uri,"http",4)==0) {
		/* if no port */
		if (!port)
			/* Default */
			port = 80;
		/* Play */
		rtsp_tunnel(chan,ip,port,url);

	} else if (strncmp(uri,"rtsp",4)==0) {
		/* if no port */
		if (!port)
			/* Default */
			port = 554;
		/* Play */
		rtsp_play(chan,ip,port,url);

	} else
		ast_log(LOG_ERROR,"RTSP ERROR: Unknown protocol in uri %s\n",uri);
	
	/* Unlock module*/
	ast_module_user_remove(u);

	/* Free ip */
	free(ip);

	/* Exit */
	return 0;
}


static int unload_module(void)
{
	int res;

	res = ast_unregister_application(name_rtsp);

	ast_module_user_hangup_all();

	return res;
}

static int load_module(void)
{
	return ast_register_application(name_rtsp, app_rtsp, syn_rtsp, des_rtsp);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "RTSP applications");

