/*
 * Asterisk -- An open source telephony toolkit.
 *
 * The AMR code is from 3GPP TS 26.104.  Copyright information for that package is available
 * in the amr directory.
 *
 * Copyright (C) 2007, Digital Solutions
 * Paul Bagyenda <bagyenda@dsmagic.com>
 * Bugfixing: Copyright (C) 2007, enum.at
 * Klaus Darilion <klaus.darilion@enum.at>
 * 
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
 * \brief Translate between signed linear and Adaptive Multi-Rate (AMR) Narrow Band (RFC 3267 format).
 *
 * \ingroup codecs
 */

/*** MODULEINFO
	
 ***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision$")

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "asterisk/lock.h"
#include "asterisk/translate.h"
#include "asterisk/config.h"
#include "asterisk/options.h"
#include "asterisk/module.h"
#include "asterisk/logger.h"
#include "asterisk/channel.h"
#include "asterisk/utils.h"

#include "amr/typedef.h"
#include "amr/interf_enc.h"
#include "amr/interf_dec.h"


/* Sample frame data */
#include "slin_amr_ex.h"
#include "amr_slin_ex.h"

#define SAMPLES_PER_SEC_NB   8000 /* 8kHz speech gives us 8000 samples per second */
#define BUFFER_SAMPLES	     8000 /* maximum number of samples we will process at a go. */
#define AMR_SAMPLES	     160
#define AMR_MAX_FRAME_LEN    32
#define AMR_MAX_FRAMES_NB (BUFFER_SAMPLES*1000)/(SAMPLES_PER_SEC_NB*20) /* each frame is 20ms, hence max frames = samples/samples_per_sec*/

static int dtx = 0;
static enum Mode enc_mode = MR122; 

/* whether we are parsing/encoding using octet-aligned mode -- XXX not very clean 
 * Note: We don't handle crc or inter-leaving for now
 */
static int octet_aligned = 0;

/* Size of (octet-aligned) speech block for each mode */
/* static short block_size[16]={12, 13, 15, 17, 19, 20, 26, 31, 5}; */

/* Taken from Table 2, of 3GPP TS 26.101, v5.0.0 */
static int num_bits[16] = {95, 103, 118, 134,148,159,204,244};

/* Mapping of encoding mode to AMR codec mode */
static const short modeConv[]={475, 515, 59, 67, 74, 795, 102, 122};

struct amr_translator_pvt {	/* both amr2lin and lin2amr */
     int *destate;  /* decoder state */
     int *enstate;  /* encoder state. */
     enum Mode enc_mode;                /* Currrently requested mode */
     int16_t buf[BUFFER_SAMPLES];	/* lin2amr, temporary storage */
     
     unsigned char pheader[AMR_MAX_FRAMES_NB + 1]; /* lin2amr temporary storage for Pay load header + Table of Contents */
     unsigned char speech_bits[AMR_MAX_FRAMES_NB*AMR_MAX_FRAME_LEN + 1]; /* storage for packed bits. */
};


/* Copy bits numbits from bits into dst. Bits are packed from and to higher order bits first. 
 * 'pos' gives start position in dst of copy. 
 * dst[] must be big enough or an unchecked error occurs. 
 */
static unsigned int pack_bits(unsigned char dst[], unsigned pos, unsigned char bits[],  unsigned numbits)
{
     unsigned x = 0;
     unsigned dbyte, doffset;
     unsigned char ch;
     unsigned int ct = 0;
     
     while (numbits >= 8) { /* first copy whole bytes. */
	  dbyte   = pos / 8;
	  doffset = 8 - (pos % 8);
	  ch = bits[x];
	  
	  dst[dbyte] = (ch>>(8-doffset)) | (dst[dbyte] & (0xffu<<doffset)); /* clear the bits we need to update, and add the new bits */
	  if (pos % 8)
	       dst[dbyte+1] = (ch<<doffset) | (dst[dbyte+1] & (0xffu>>(8-doffset))); /* Put rest of bits in next bin. */

	  pos += 8;
	  numbits -= 8;
	  ct += 8;
	  x++;
     }
     
     if (numbits > 0) {
	  dbyte   = pos / 8;
	  doffset = 8 - (pos % 8);
	  ch = bits[x] & (0xffu<<(8-numbits)); /* mask off everything else */
	  
	  dst[dbyte] = (ch>>(8-doffset)) | (dst[dbyte] & (0xffu<<doffset)); /* clear the bits we need to update, and add the new bits */

	  pos += numbits;	  
	  if (pos / 8 != dbyte)
	       dst[dbyte+1] = (ch<<doffset) | (dst[dbyte+1] & (0xffu>>(8-doffset))); /* Put rest of bits in next bin. */

	  ct += numbits;
     }
     return ct;
}

/* Copy bits from src to bytes (destination), higher order bits first. Kind of the reverse of the above */

static unsigned int unpack_bits(unsigned char bytes[], unsigned char src[], unsigned pos, unsigned numbits)
{
     unsigned x = 0;
     unsigned dbyte, doffset;
     unsigned int ct = 0;
     
     while (numbits >= 8) { /* first copy whole bytes. */
	  dbyte   = pos / 8;
	  doffset = 8 - (pos % 8);
	  
	  bytes[x] = src[dbyte]<<(8-doffset);
	  if (pos % 8)
	       bytes[x] |= src[dbyte+1]>>doffset; /* Put rest of bits in. */

	  pos += 8;
	  numbits -= 8;
	  ct += 8;
	  x++;
     }
     
     if (numbits > 0) {
	  dbyte   = pos / 8;
	  doffset = 8 - (pos % 8);
	  
	  bytes[x] = src[dbyte]<<(8-doffset);

	  pos += numbits;	  
	  if (pos / 8 != dbyte)
	       bytes[x] |= src[dbyte+1]>>doffset;

	  bytes[x] &= 0xff<<(8-numbits); /* clear out any excess. */

	  ct += numbits;
     }
     return ct;
}

/* XXX only bandwidth efficient mode is supported for now. Other one
 * requires better SDP handling in Asterisk 
 */
static int amr_new(struct ast_trans_pvt *pvt)
{
	struct amr_translator_pvt *tmp = pvt->pvt;
	
	tmp->enstate = Encoder_Interface_init(dtx);
	tmp->destate = Decoder_Interface_init();
	tmp->enc_mode = enc_mode;
	return 0;
}

/* XXX this sample may not be transmitted as it is not in RTP packet format! */
static struct ast_frame *lintoamr_sample(void)
{
	static struct ast_frame f;
	f.frametype = AST_FRAME_VOICE;
	f.subclass = AST_FORMAT_SLINEAR;
	f.datalen = sizeof(slin_amr_ex);
	/* Assume 8000 Hz */
	f.samples = sizeof(slin_amr_ex)/2;
	f.mallocd = 0;
	f.offset = 0;
	f.src = __PRETTY_FUNCTION__;
	f.data = slin_amr_ex;
	return &f;
}

static struct ast_frame *amrtolin_sample(void)
{
	static struct ast_frame f;
	f.frametype = AST_FRAME_VOICE;
	f.subclass = AST_FORMAT_AMRNB;
	f.datalen = sizeof(amr_slin_ex);
	/* All frames are 20 ms long */
	f.samples = AMR_SAMPLES;
	f.mallocd = 0;
	f.offset = 0;
	f.src = __PRETTY_FUNCTION__;
	f.data = amr_slin_ex;
	return &f;
}

/*! \brief decode and store in outbuf. */
static int amrtolin_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct amr_translator_pvt *tmp = pvt->pvt;
	int x = 0, more_frames = 1, num_frames = 0;
	int16_t *dst = (int16_t *)pvt->outbuf;
	unsigned char *src = f->data, cmr, buffer[AMR_MAX_FRAME_LEN], toc_entries[AMR_MAX_FRAMES_NB];
	unsigned int pos; /* position in the bit stream. */

	cmr = (src[0]) >> 4;	
	pos = octet_aligned ? 8 : 4;

	/* Get the table of contents first... */
	while (((pos + 7)/8 < f->datalen) && more_frames) {
		pos += unpack_bits(&toc_entries[num_frames], src, pos, octet_aligned ? 8 : 6); /* get table of contents. */

		more_frames = (toc_entries[num_frames]>>7);	     
		toc_entries[num_frames] &= ~(1<<7); /* Set top bit to 0 */

		/* ast_verbose("amrtolin_framein: cmr=%02hhx, toc=%02hhx, more=%d, datalen=%d\n",
			cmr, toc_entries[num_frames], more_frames, f->datalen); */

		num_frames++;
	}
	
	/* Now get the speech bits, and decode as we go. */
	for (x = 0; x<num_frames; x++) {
		unsigned ft = (toc_entries[x] >> 3) & 0x0f;

		/* XXX maybe we don't need to check */
		if (pvt->samples + AMR_SAMPLES > BUFFER_SAMPLES) {	
			ast_log(LOG_WARNING, "Out of buffer space\n");
			return -1;
		}
	     
		memset(buffer, 0, sizeof buffer); /* clear it. */

		buffer[0] = toc_entries[x];
		/* for octet-aligned mode, the speech frames are octet aligned as well */
		pos += unpack_bits(buffer+1, src, pos, 
			octet_aligned ? (num_bits[ft]+7)&~7 : num_bits[ft]);

		Decoder_Interface_Decode(tmp->destate,buffer, dst + pvt->samples,0);

		/* ast_verbose("amrtolin_framein: Decode toc=%02hhx, ft=%u, num_bits=%d\n",
			toc_entries[x], ft, num_bits[ft]); */

		pvt->samples += AMR_SAMPLES;
		pvt->datalen += 2 * AMR_SAMPLES;
	}

	/* Honour the requested codec? */
	if (cmr <= tmp->enc_mode) {
		/* ast_verbose("amrtolin_framein: setting encoder mode to %d\n",cmr); */
		tmp->enc_mode = cmr;
	}

	return 0;
}

/*! \brief store samples into working buffer for later decode */
static int lintoamr_framein(struct ast_trans_pvt *pvt, struct ast_frame *f)
{
	struct amr_translator_pvt *tmp = pvt->pvt;

	/* XXX We should look at how old the rest of our stream is, and if it
	   is too old, then we should overwrite it entirely, otherwise we can
	   get artifacts of earlier talk that do not belong */

	/* ast_verbose("lintoamr_framein: %d samples\n", f->samples); */

	if (pvt->samples + f->samples > BUFFER_SAMPLES) {
		ast_log(LOG_WARNING, "Out of buffer space\n");
		return -1;
	}
	memcpy(tmp->buf + pvt->samples, f->data, f->datalen);
	pvt->samples += f->samples;
	pvt->datalen += 2*f->samples;

	return 0;
}

/*! \brief encode and produce a frame */
static struct ast_frame *lintoamr_frameout(struct ast_trans_pvt *pvt)
{
	struct amr_translator_pvt *tmp = pvt->pvt;
	int datalen = 0, samples = 0, npad;
	unsigned int pbits = 0, sbits = 0; /* header and body bit count */
	unsigned char buffer[AMR_MAX_FRAME_LEN], cmr = tmp->enc_mode, toc_entry, xzero = 0;
	unsigned char mode;

	/* ast_verbose("lintoamr_frameout: %d samples to process\n", pvt->samples); */
	
	/* We can't work on anything less than a frame in size */
	if (pvt->samples < AMR_SAMPLES)
		return NULL;

	/* First, put the CMR into the header. */
	cmr <<= 4; /* Put in higher order nibble. */
	pbits += pack_bits(tmp->pheader, pbits, &cmr, octet_aligned ? 8 : 4);

	while (pvt->samples >= AMR_SAMPLES) {	     
		/* Encode a frame of data */
	     int byte_count = Encoder_Interface_Encode(tmp->enstate, tmp->enc_mode, 
				      tmp->buf + samples, 
				      buffer, 0);
	     
	     samples += AMR_SAMPLES;
	     pvt->samples -= AMR_SAMPLES;
	     pvt->datalen -= 2*AMR_SAMPLES;
	     
	     toc_entry = buffer[0];
	     /* Set the F bit */
	     if (pvt->samples >= AMR_SAMPLES) /* then we have another frame to  pack, so... */
		  toc_entry |= (1<<7);
	     pbits += pack_bits(tmp->pheader, pbits, &toc_entry, octet_aligned ? 8 : 6); /* put in the table of contents element. */
	     
	     /* Pack the bits of the speech. */
	     mode = (toc_entry>>3) & 0x0F;
	     sbits += pack_bits(tmp->speech_bits, sbits, buffer + 1, 
				octet_aligned ? (num_bits[mode]+7)&~7 : num_bits[mode]); 
	     
		/* ast_verbose("codec_amr: encoded %d bytes, mode = %d, samples left=%d, first two bytes =%02hhx:%02hhx\n", 
			byte_count, tmp->enc_mode, pvt->samples, buffer[1], buffer[2]); */
	}
	
    /* Finally, put the header and the speech together into the pvt->buffer */
	/* ast_verbose("codec_amr: building buffer: pbits=%d, sbits=%d\n",pbits,sbits); */
	
	pack_bits((unsigned char *)pvt->outbuf, 0, tmp->pheader, pbits);
	pack_bits((unsigned char *)pvt->outbuf, pbits, tmp->speech_bits, sbits);
	npad = 8 - ((sbits + pbits) % 8); /* Number of padding bits */
	if (npad == 8) {
		npad = 0;
	}

	if (octet_aligned && npad != 0) {
		if (option_verbose > 2)
			ast_verbose(VERBOSE_PREFIX_3 "lintoamr_frameout: ERROR: Padding bits cannot be > 0 in octet aligned mode!\n");
		/* assert("Padding bits cannot be > 0 in octet aligned mode! " && 0); */
		return NULL;
	}

	if (npad) {
		pack_bits((unsigned char *)pvt->outbuf, pbits+sbits, &xzero, npad); /* zero out the rest of the padding bits. */
	}
	datalen = (sbits + pbits  + 7)/8; /* Round up to nearest octet. */
	
	/* ast_verbose("codec_amr: Totally encoded %d bytes worth, mode = %d, samples left=%d\n", 
		datalen, tmp->enc_mode, pvt->samples); */
	
	/* Move the data at the end of the buffer to the front */
	if (pvt->samples)
	     memmove(tmp->buf, tmp->buf + samples, pvt->samples * 2);
	
	return ast_trans_frameout(pvt, datalen, samples);
}

static void amr_destroy_stuff(struct ast_trans_pvt *pvt)
{
	struct amr_translator_pvt *tmp = pvt->pvt;
	Encoder_Interface_exit(tmp->enstate);
	Decoder_Interface_exit(tmp->destate);
}

static struct ast_translator amrtolin = {
	.name = "amrtolin", 
	.srcfmt = AST_FORMAT_AMRNB,
	.dstfmt = AST_FORMAT_SLINEAR,
	.newpvt = amr_new,
	.framein = amrtolin_framein,
	.destroy = amr_destroy_stuff,
	.sample = amrtolin_sample,
	.buffer_samples = BUFFER_SAMPLES,
	.buf_size = BUFFER_SAMPLES * 2,
	.desc_size = sizeof (struct amr_translator_pvt )
};

static struct ast_translator lintoamr = {
	.name = "lintoamr", 
	.srcfmt = AST_FORMAT_SLINEAR,
	.dstfmt = AST_FORMAT_AMRNB,
	.newpvt = amr_new,
	.framein = lintoamr_framein,
	.frameout = lintoamr_frameout,
	.destroy = amr_destroy_stuff,
	.sample = lintoamr_sample,
	.desc_size = sizeof (struct amr_translator_pvt ),
	.buf_size = (BUFFER_SAMPLES * AMR_MAX_FRAME_LEN + AMR_SAMPLES - 1)/AMR_SAMPLES,
};


static void parse_config(void)
{    
	struct ast_variable *var;
	struct ast_config *cfg = ast_config_load("codecs.conf");
	if (cfg) {
		if (option_verbose > 2)
			ast_verbose(VERBOSE_PREFIX_3 "codec_amr: parsing codecs.conf\n");
		for (var = ast_variable_browse(cfg, "amr"); var; var = var->next) {
			if (!strcasecmp(var->name, "octet-aligned")) {
				octet_aligned = atoi(var->value);
			} else if (!strcasecmp(var->name, "dtx")) {
				dtx = atoi(var->value);
			} else if (!strcasecmp(var->name, "mode")) {
				int mode_tmp = strtol(var->value + 2, NULL, 10);
				int req_mode;
				for (req_mode = 0; req_mode < 8; req_mode++) {
					if (mode_tmp == modeConv[req_mode])
						break;
				}
				if (req_mode == 8) {
					ast_log(LOG_ERROR, "Error, unknown mode %s. Must be one of MR475, MR515, MR59, MR67, MR74, MR795, MR102, MR122",
						var->value);
				} else {
					enc_mode = req_mode;
				}
			}
		}
	} else {
		if (option_verbose > 2)
			ast_verbose(VERBOSE_PREFIX_3 "codec_amr: codecs.conf not found, using default values\n");
	}
	if (option_verbose > 2) {
		ast_verbose(VERBOSE_PREFIX_3 "codec_amr: set octed-aligned mode to %d\n", octet_aligned);
		ast_verbose(VERBOSE_PREFIX_3 "codec_amr: set dtx mode to %d\n", dtx);
		ast_verbose(VERBOSE_PREFIX_3 "codec_amr: AMR mode set to MR%d (%d)\n", modeConv[enc_mode],enc_mode);
	}
	ast_config_destroy(cfg);
	ast_verbose("codec_amr: enc_mode = %d, dtx = %d\n", enc_mode, dtx);
}

/*! \brief standard module glue */
static int reload(void)
{
	parse_config();
	return 0;
}

static int unload_module(void)
{
	int res;

	res = ast_unregister_translator(&lintoamr);
	if (!res)
		res = ast_unregister_translator(&amrtolin);

	return res;
}

static int load_module(void)
{
	int res;

	parse_config();
	res = ast_register_translator(&amrtolin);
	if (!res) 
		res=ast_register_translator(&lintoamr);
	else
		ast_unregister_translator(&amrtolin);

	return res;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "AMR Coder/Decoder",
		.load = load_module,
		.unload = unload_module,
		.reload = reload,
	       );
