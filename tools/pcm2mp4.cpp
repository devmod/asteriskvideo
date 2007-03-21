/* pcm2mp4
 * Tool for converting a pcmu audio file to mp4
 * Copyright (C) 2006 Sergio Garcia Murillo
 *
 * sergio.garcia@fontventa.com
 * http://sip.fontventa.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <mp4.h>

char* p3gppSupportedBrands[2] = {"3gp5", "3gp4"};

int main(int argc,char **argv)
{

	/* Check args */
	if (argc<3)
	{
		printf("rtp2mp4\nusage: rtp2mp4 audio output\n");
		return -1;
	}

	/* Get names */
	char *inputAudio = argv[1];
	char *output = argv[2];

	/* Open audio */
	int fdAudio = open(inputAudio,O_RDONLY);

	/* Check */
	if (fdAudio<0)
	{
		printf("Couldn't open input audio file %s [%d]\n",inputAudio,errno);
		return -1;
	}

	/* Create mp4 output file */
	MP4FileHandle mp4 = MP4CreateEx(output);/*
					0,
					0,  // createFlags,
					1,  // add ftyp atom
					0,  // don't add iods
			
					p3gppSupportedBrands[0],
					0x0001,
					p3gppSupportedBrands,
					sizeof(p3gppSupportedBrands) / sizeof(p3gppSupportedBrands[0])
				);*/

	/* Check result */
	if (mp4==MP4_INVALID_FILE_HANDLE)
	{
		printf("Couldn't open output file %s\n",output);
		return -1;
	}

	/* Create audio track */
	MP4TrackId audio = MP4AddAudioTrack(mp4,8000,480,MP4_ULAW_AUDIO_TYPE);
	
	/* Create hint track for aufio */
	MP4TrackId hintAudio = MP4AddHintTrack(mp4,audio);

	/* Set audio payload */
	unsigned char type = 0;
	MP4SetHintTrackRtpPayload(mp4,hintAudio,"PCMU",&type,0,NULL,1,0);


	/* Data */
	unsigned char rtp[1500];
	unsigned char frame[65535];
	int length = 0;
	int size;
	bool first = true;
	int sampleId = 1;

	/* Read from input */
	while(true)
	{
		/* Set rtp size */
		size = 160;

		/* Read data */
		if(!read(fdAudio,rtp,size)>0)
			break;

		/* Add hint */
		MP4AddRtpHint(mp4,hintAudio);

		/* Add packet */
		MP4AddRtpPacket(mp4,hintAudio,1);

		/* Set rtp reference for frame data */
		MP4AddRtpSampleData(mp4,hintAudio,sampleId,0,size);

		/* Write rtp */
		MP4WriteRtpHint(mp4,hintAudio,size,1);

		/* Save audio data */
		MP4WriteSample(mp4,audio,rtp,size,size,0,1);

		/* Next */
		sampleId++;
	}

	/* Close */
	MP4Close(mp4);
	close(fdAudio);

	/* End */
	return 0;
}
