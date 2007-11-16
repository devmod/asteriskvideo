/* pcm2mp4
 * Video bandwith control for mp4
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

int main(int argc,char **argv)
{
	/* Check args */
	if (argc<2)
	{
		printf("mp4band\nusage: mp4band file\n");
		return -1;
	}

	/* Open mp4*/
	MP4FileHandle mp4 = MP4Read(argv[1], 9);

	/* Find first hint track */
	MP4TrackId trackId = MP4FindTrackId(mp4, 0, MP4_VIDEO_TRACK_TYPE, 0);

	/* If not found video track*/
	if (trackId!=MP4_INVALID_TRACK_ID)
	{
		unsigned int frameTotal = MP4GetTrackNumberOfSamples(mp4,trackId);

		printf("Number of samples: %d\n", frameTotal);

		/* Iterate frames */
		for (int i=1;i<frameTotal+1;i++)
		{
			/* Get size of sample */
			unsigned int frameSize = MP4GetSampleSize(mp4, trackId, i);

			/* Get sample timestamp */
			unsigned int frameTime = MP4GetSampleTime(mp4, trackId, i);

			printf("%d\t%d\t%d\t%d\n",i,frameTime,frameSize,frameSize*8/10);
		}
	}

	/* Close */
	MP4Close(mp4);

	/* End */
	return 0;
}
