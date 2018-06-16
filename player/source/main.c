#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>

#include "monorale.h"
#define DEBUG_INST /* minor profiling */
#define N3DS_TEST
#define CHANNEL 0x08

volatile bool runThreads = true;
static const size_t buffSize = 32768;
static OggVorbis_File vorbisFile;
static vorbis_info *vi;
static FILE *f;
int isVorbis(const char *in)
{
	FILE *ft = fopen(in, "r");
	OggVorbis_File testvf;
	int err;

	if(ft == NULL)
		return -1;

	err = ov_test(ft, &testvf, NULL, 0);

	ov_clear(&testvf);
	fclose(ft);
	return err;
}


int initVorbis(){	
	//printf("opening %s...\n",file);
	//printf("test %s ret:%d\n",file,isVorbis(file));
	if((f = fopen("romfs:/ba.ogg","rb"))==NULL) return 1;
	//printf("ov_test:%d",ov_test(f,&vorbisFile,NULL,0));
	int ov_r = ov_open(f,&vorbisFile,NULL,0);
	printf("ov_r:%d\n",ov_r);
	if(ov_r < 0) return 2;
	if((vi = ov_info(&vorbisFile,-1))==NULL) return 3;
	return 0;
}

uint64_t fillVorbisBuffer(void* bufferOut)
{
	uint64_t samplesRead = 0;
	int samplesToRead = buffSize;

	while(samplesToRead > 0)
	{
		static int current_section;
		int samplesJustRead =
			ov_read(&vorbisFile, bufferOut,
					samplesToRead > 4096 ? 4096	: samplesToRead,
					&current_section);

		if(samplesJustRead < 0)
			return samplesJustRead;
		else if(samplesJustRead == 0)
		{
			/* End of file reached. */
			break;
		}

		samplesRead += samplesJustRead;
		samplesToRead -= samplesJustRead;
		bufferOut += samplesJustRead;
	}

	return samplesRead / sizeof(int16_t);
}


void initSound(){
	Result res;
	res = ndspInit();
	printf("initSound()->ndspInit(): %08lx\n",res);
	//res = romfsInit();
	//printf("initSound()->romfsInit(): %08lx\n",res);
	//int r = initVorbis(fp);
	//printf("r:%d\n",r);
	//if(r) {
		//svcBreak(USERBREAK_ASSERT);
	//	printf("load failed!\n");
	//}
	//buffer1 = linearAlloc(buffSize*sizeof(int16_t));
	//buffer2 = linearAlloc(buffSize*sizeof(int16_t));
	//printf("Buffer LM allocated\n");
	//ndspSetMasterVol(1.0);
	//ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnReset(CHANNEL);
	ndspChnWaveBufClear(CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL,NDSP_INTERP_LINEAR);
	ndspChnSetRate(CHANNEL,44100);
	ndspChnSetFormat(CHANNEL,NDSP_FORMAT_STEREO_PCM16);
	printf("InitSound Done.\n");
}


void soundThread(void *arg) {
	//initSound();
	ndspWaveBuf waveBuf[2];
	bool lastbuf = false;
	int16_t* buffer1 = linearAlloc(buffSize * sizeof(int16_t));
	int16_t* buffer2 = linearAlloc(buffSize * sizeof(int16_t));
	printf("Buffer LM allocated\n");
	memset(waveBuf,0,sizeof(waveBuf));
	
	waveBuf[0].nsamples = fillVorbisBuffer(&buffer1[0])/2;
	waveBuf[0].data_vaddr = &buffer1[0];
	ndspChnWaveBufAdd(CHANNEL,&waveBuf[0]);

	waveBuf[1].nsamples = fillVorbisBuffer(&buffer2[0])/2;
	waveBuf[1].data_vaddr = &buffer2[0];
	ndspChnWaveBufAdd(CHANNEL,&waveBuf[1]);
	
	while(ndspChnIsPlaying(CHANNEL) == false);
	svcSleepThread(100*100);
	while(runThreads){
		//svcSleepThread(100 * 1000);
		if(lastbuf == true && waveBuf[0].status == NDSP_WBUF_DONE && waveBuf[1].status == NDSP_WBUF_DONE) break;
		if(lastbuf == true) continue;
		
		if(waveBuf[0].status == NDSP_WBUF_DONE) {
			//printf("wb0\n");
			size_t read = fillVorbisBuffer(&buffer1[0]);
			if(read <= 0) {
				lastbuf = true;
				continue;
			} else if(read < buffSize)
				waveBuf[0].nsamples = read/2;
			ndspChnWaveBufAdd(CHANNEL,&waveBuf[0]);
		}

		if(waveBuf[1].status == NDSP_WBUF_DONE) {
			//intf("wb1\n");
			size_t read = fillVorbisBuffer(&buffer2[0]);
			if(read <= 0)
			{
				lastbuf = true;
				continue;
			} else if(read < buffSize) waveBuf[1].nsamples = read/2;
			ndspChnWaveBufAdd(CHANNEL, &waveBuf[1]);
		}
		DSP_FlushDataCache(buffer1,buffSize*sizeof(int16_t));
		DSP_FlushDataCache(buffer2,buffSize*sizeof(int16_t));
	}
	
	//CleanUP
	ndspChnWaveBufClear(CHANNEL);
	ndspExit();
	printf("Bye NDSP!\n");
	linearFree(buffer1);
	linearFree(buffer2);
	printf("Freed buffers\n");
	threadExit(0);
	//return;
}	



int main(int argc, char **argv)
{
	Result res;
	FILE *vid;
	size_t vid_sz, frame;
	monorale_hdr *vid_hdr;
	Thread thr;
	#ifdef DEBUG_INST
	u64 tot_ticks, min_ticks, max_ticks, dif_ticks;
	#endif /* DEBUG_INST */

	res = romfsInit();
	if (R_FAILED(res)) return 1;

	vid = fopen("romfs:/monorale.bin", "rb");
	if (vid == NULL) return 1;

	fseek(vid, 0L, SEEK_END);
	vid_sz = ftell(vid);
	fseek(vid, 0L, SEEK_SET);

	vid_hdr = malloc(vid_sz);
	if (vid_hdr == NULL) return 1;

	gfxInit(GSP_RGB565_OES, GSP_RGB565_OES, false);
	consoleInit(GFX_BOTTOM, NULL);
	
	#ifdef N3DS_TEST
	u8 model;
	res = ptmSysmInit();
	printf("ptmSysmInit(): %08lX\n",res);
	res = cfguInit();
	printf("cfguInit(): %08lX\n",res);
	res = CFGU_GetSystemModel(&model);
	printf("CFGU_GetSystemModel(): %08lx\n",res);
	if(res ||model == 2 || model >= 4){
		res = PTMSYSM_ConfigureNew3DSCPU(0x3);
		printf("ConfigN3DSCPU(): %08lX\n",res);
	}
	#endif
	
	printf("Starting...\n");
	fread(vid_hdr, vid_sz, 1, vid);
	fclose(vid);
	
	initVorbis();
	//sound = fopen("romfs:/ba.ogg","rb");
	//if(sound == NULL){
	//	printf("Load Failed!");
	//}
	initSound();
	
	#ifdef DEBUG_INST
	tot_ticks = 0;
	min_ticks = ~0ULL;
	max_ticks = 0;
	#endif /* DEBUG_INST */
	
	s32 prio = 0;
	svcGetThreadPriority(&prio,CUR_THREAD_HANDLE);
	printf("Main prio: 0x%lx\n",prio);
	thr = threadCreate(soundThread,NULL,16384,prio-1,-2,false);
	printf("Thread Created\n");
	frame = 0;
	while(aptMainLoop() && frame < monorale_frames(vid_hdr)) {
		u16 *fb;

		gspWaitForVBlank();

		hidScanInput();
		if (hidKeysDown() & KEY_B) break;

		#ifdef DEBUG_INST
		dif_ticks = svcGetSystemTick();
		#endif /* DEBUG_INST */

		fb = (u16*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		monorale_doframe(vid_hdr, frame, fb);

		#ifdef DEBUG_INST
		dif_ticks = svcGetSystemTick() - dif_ticks;
		min_ticks = (dif_ticks < min_ticks) ? dif_ticks : min_ticks;
		max_ticks = (dif_ticks > max_ticks) ? dif_ticks : max_ticks;
		tot_ticks += dif_ticks;
		#endif /* DEBUG_INST */
		gspWaitForVBlank(); /* sync it up to 30fps */

		gfxFlushBuffers();
		gfxSwapBuffers();
		frame++;
	}

	#ifdef DEBUG_INST
	printf("frames = %d, ticks = %llu, min_ticks = %llu,"
			"max_ticks = %llu, avg_ticks = %llu\n",
			frame, tot_ticks, min_ticks, max_ticks,
			tot_ticks / frame);

	printf("min_ticks = %f ms, max_ticks = %f ms, avg_ticks = %f ms\n",
			(double)min_ticks / CPU_TICKS_PER_MSEC,
			(double)max_ticks / CPU_TICKS_PER_MSEC,
			(double)(tot_ticks / frame) / CPU_TICKS_PER_MSEC);
	#endif /* DEBUG_INST */
	runThreads = false;
	free(vid_hdr);
	threadJoin(thr,U64_MAX);
	threadFree(thr);
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		if (hidKeysDown() & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
	return 0;
}
