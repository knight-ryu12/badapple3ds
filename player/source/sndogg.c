#include <3ds.h>

#include "sndogg.h"

static const size_t buffSize = 44100;

Result initSound(u16 sample_rate)
{
	Result res;
	res = ndspInit();
	if (R_FAILED(res)) return res;

	ndspChnReset(0);
	ndspChnWaveBufClear(0);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(0, sample_rate);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
	return 0;
}

int initVorbis(const char *path, OggVorbis_File *vorbisFile, vorbis_info **vi)
{
	int ov_res;
	FILE *f = fopen(path, "rb");
	if (f == NULL) return 1;

	ov_res = ov_open(f, vorbisFile, NULL, 0);
	if (ov_res < 0) {
		fclose(f);
		return ov_res;
	}

	*vi = ov_info(vorbisFile, -1);
	if (*vi == NULL) return 3;

	return 0;
}

size_t fillVorbisBuffer(int16_t *buf, size_t samples, OggVorbis_File *vorbisFile)
{
	size_t samplesRead = 0;
	int samplesToRead = samples;

	while(samplesToRead > 0)
	{
		int current_section;
		int samplesJustRead =
			ov_read(vorbisFile, (char*)buf,
					samplesToRead > 4096 ? 4096	: samplesToRead,
					&current_section);

		if(samplesJustRead < 0)
			return samplesJustRead;
		else if(samplesJustRead == 0)
			break;
			/* End of file reached. */

		samplesRead += samplesJustRead;
		samplesToRead -= samplesJustRead;
		buf += samplesJustRead / 2;
	}

	return samplesRead / sizeof(int16_t);
}

void soundThread(void *arg) {
	
	printf("Hello from soundThread!\n");

	OggVorbis_File *vorbisFile = (OggVorbis_File*)arg;
	ndspWaveBuf waveBuf[2];
	int16_t *samplebuf;
	int cur_wvbuf = 0;
	
	samplebuf = linearAlloc(buffSize * sizeof(int16_t) * 2);
	memset(waveBuf, 0, sizeof(waveBuf));

	waveBuf[0].data_vaddr = samplebuf;
	waveBuf[1].data_vaddr = samplebuf + buffSize;

	waveBuf[0].status = NDSP_WBUF_DONE;
	waveBuf[1].status = NDSP_WBUF_DONE;
	
	//svcSleepThread(100*1000); // hack

	while(runSound) {
		while(runSound && !playSound)
			svcSleepThread(10e9 / 60);

		int16_t *cursamplebuf = (int16_t*)waveBuf[cur_wvbuf].data_vaddr;

		waveBuf[cur_wvbuf].nsamples = fillVorbisBuffer(cursamplebuf, buffSize, vorbisFile) / 2;
		if (waveBuf[cur_wvbuf].nsamples == 0) break;

		DSP_FlushDataCache(cursamplebuf, buffSize * sizeof(int16_t));
		ndspChnWaveBufAdd(0, &waveBuf[cur_wvbuf]);

		cur_wvbuf ^= 1;
		while(waveBuf[cur_wvbuf].status != NDSP_WBUF_DONE && runSound)
			svcSleepThread(10e9 / (buffSize*4));
	}

	// cleanup
	ndspChnWaveBufClear(0);
	ndspExit();
	linearFree(samplebuf);
	printf("Bye!\n");
	runSound = 0;
	threadExit(0);
}
