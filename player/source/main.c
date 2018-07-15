#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>

#include "monorale.h"
#include "sndogg.h"

#define DEBUG // For debug string.

volatile u32 runSound, playSound;
volatile u32 runVideo, playVideo;

bool do_new_speedup(void)
{
	Result res;
	u8 model;

	res = ptmSysmInit();
	if (R_FAILED(res)){ 
		#ifdef DEBUG
			printf("ptmSysmInit(): %08lx\n",res);
		#endif
		return 0;
	}
	res = cfguInit();
	if (R_FAILED(res)) return 0;
	CFGU_GetSystemModel(&model);
	if (model == 2 || model >= 4)
		PTMSYSM_ConfigureNew3DSCPU(3);
	ptmSysmExit();
	cfguExit();
	return model;
}

void threadPlayStopHook(APT_HookType hook, void *param)
{
	switch(hook) {
		case APTHOOK_ONSUSPEND:
		case APTHOOK_ONSLEEP:
			playSound = playVideo = 0;
			break;

		case APTHOOK_ONRESTORE:
		case APTHOOK_ONWAKEUP:
			playSound = playVideo = 1;
			break;

		case APTHOOK_ONEXIT:
			runSound = playSound = 0;
			runVideo = playVideo = 0;
			break;

		default:
			break;
	}
}

int main(int argc, char **argv)
{
	Result res;
	s32 main_prio;
	Thread vid_thr, snd_thr;
	aptHookCookie thr_playhook;

	OggVorbis_File vorbisFile;
	vorbis_info *vi;
	monorale_hdr *video;

	gfxInit(GSP_RGB565_OES, GSP_RGB565_OES, false);
	consoleInit(GFX_BOTTOM, NULL);

	//printf("ConfigN3DSCPU(): %08X\n\n\n", do_new_speedup());


	printf("\e[0;33mWolfvak\e[0;37m and \e[0;34mChromaryu\e[0;37m presents:\n");
	printf("\e[0;90mBad \e[0;37mApple!!! PV on 3ds!\n");
	printf("Song: Bad Apple!!! feat nomico\n");

	res = romfsInit();
	if (R_FAILED(res)) {
		printf("Error at romfsInit()!? res:%08lx\n",res); // Throwing Error
		goto end; // Goto end (wait for input)
	}

	printf("romfsInit(): %08lx\n",res);
	res = aptInit();

	if (R_FAILED(res)) {
		printf("Error at aptInit()!? res:%08lx\n",res); 
		goto end;
	}
	printf("aptInit(): %08lx\n",res);

	res = initVorbis("romfs:/ba.ogg", &vorbisFile, &vi);
	if (res != 0) {
		printf("Error at initVorbis()!?\n");
		goto end;
	}
	printf("[\e[0;32mSUCCESS\e[0;37m] initVorbis()\n");
	video = monorale_init("romfs:/monorale.bin");
	
	if (video == NULL) {
		printf("Error at monorale_init()!?\n");
		goto end;
	}

	printf("[\e[0;32mSUCCESS\e[0;37m]monorale_init()\n");

	initSound(vi->rate);
	printf("[\e[0;32mSUCCESS\e[0;37m]initSound()\n");

	svcGetThreadPriority(&main_prio, CUR_THREAD_HANDLE);
	printf("mainThreadPrio = 0x%lx\n",main_prio);
	if (main_prio < 2) svcBreak(USERBREAK_ASSERT);

	aptHook(&thr_playhook, threadPlayStopHook, NULL);
	
	printf("aptHook() called\n");

	res = APT_SetAppCpuTimeLimit(30);
	printf("SetAppCpuTimeLimit(): %08lx\n",res);

	runSound = playSound = 1;
	runVideo = playVideo = 1;
	snd_thr = threadCreate(soundThread, &vorbisFile, 32768, main_prio + 2, 1, true);
	vid_thr = threadCreate(monoraleThread, video, 32768, main_prio + 1, 0, true);
	
	while(aptMainLoop()) {
		svcSleepThread(10e9 / 60);

		if ((runSound | runVideo) == 0)
			break;

		hidScanInput();
		if (hidKeysDown() & KEY_START) break;
	}

	runSound = playSound = 0;
	runVideo = playVideo = 0;

	threadJoin(vid_thr, U64_MAX);
	threadJoin(snd_thr, U64_MAX);

	aptUnhook(&thr_playhook);

	fclose((FILE*)(vorbisFile.datasource)); /* hack */
	free(video);

	end:
	printf("Press start to exit...\n");
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		if (hidKeysDown() & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
	romfsExit();
	aptExit();
	return 0;
}
