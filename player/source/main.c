#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "monorale.h"
#include "sndogg.h"

volatile u32 runVideo, runSound;

bool do_new_speedup(void)
{
	Result res, model;

	res = ptmSysmInit();
	if (R_FAILED(res)) return 0;
	model = PTMSYSM_CheckNew3DS();
	if (model == 1)
		PTMSYSM_ConfigureNew3DSCPU(3);
	ptmSysmExit();
	return model;
}

int main(int argc, char **argv)
{
	Result res;
	s32 main_prio;
	Thread vid_thr, snd_thr;

	OggVorbis_File vorbisFile;
	vorbis_info *vi;
	monorale_hdr *video;

	gfxInit(GSP_RGB565_OES, GSP_RGB565_OES, false);
	consoleInit(GFX_BOTTOM, NULL);

	printf("ConfigN3DSCPU(): %08X\n\n\n", do_new_speedup());

	res = romfsInit();
	if (R_FAILED(res)) return 1;

	res = initVorbis("romfs:/ba.ogg", &vorbisFile, &vi);
	if (res != 0) return 1;

	video = monorale_init("romfs:/monorale.bin");
	if (video == NULL) return 1;

	initSound(vi->rate);

	printf("Wolfvak and Chromaryu present:\n");
	printf("Bad Apple!!! PV on 3ds!\n");
	printf("Song: Bad Apple!!! feat nomico\n");

	svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);
	if (main_prio < 2) svcBreak(USERBREAK_ASSERT);

	runSound = 1;
	runVideo = 1;
	snd_thr = threadCreate(soundThread, &vorbisFile, 32768, main_prio - 1, -2, true);
	vid_thr = threadCreate(monoraleThread, video, 32768, main_prio - 2, -2, true);

	while(aptMainLoop()) {
		svcSleepThread(10e9 / 60);

		if ((runSound | runVideo) == 0)
			break;

		hidScanInput();
		if (hidKeysDown() & KEY_B) {
			runSound = 0;
			runVideo = 0;
			break;
		}
	}

	threadJoin(vid_thr, U64_MAX);
	threadJoin(snd_thr, U64_MAX);

	fclose((FILE*)(vorbisFile.datasource)); /* hack */
	free(video);

	gfxExit();
	return 0;
}
