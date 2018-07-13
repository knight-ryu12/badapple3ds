#include <3ds.h>

#include "monorale.h"

static inline u16 *monorale_frame(monorale_hdr *hdr, size_t frame)
{
	monorale_frameinf *info;
	if (frame >= hdr->framecnt) return NULL;

	info = &hdr->inf[frame];
	return (u16*)((char*)hdr + info->offset);
}

static inline uint32_t monorale_framecnt(monorale_hdr *hdr, size_t frame)
{
	if (frame >= hdr->framecnt) return 0;
	return hdr->inf[frame].cmdcnt;
}

/* TODO: Add hardware filling */
void monorale_doframe(monorale_hdr *hdr, size_t frame, u16 *fb)
{
	u32 fill_val = 0;
	u16 *cmds = monorale_frame(hdr, frame);
	size_t cmdcnt = monorale_framecnt(hdr, frame);

	for (size_t i = 0; i < cmdcnt; i++) {
		size_t len_px = cmds[i];

		memset(fb, fill_val, len_px << 1);
		fb += len_px;
		fill_val ^= 0xFFFFFFFF;
	}
}

monorale_hdr *monorale_init(const char *path)
{
	size_t monolen;
	monorale_hdr *ret;
	FILE *f = fopen(path, "rb");
	if (f == NULL) return NULL;

	fseek(f, 0L, SEEK_END);
	monolen = ftell(f);
	fseek(f, 0L, SEEK_SET);

	ret = malloc(monolen);
	if (ret != NULL)
		fread(ret, monolen, 1, f);

	fclose(f);
	return ret;
}

void monoraleThread(void *arg)
{
	printf("Hello from video thread!\n");
	#ifdef DEBUGINST
	u64 tot_ticks, min_ticks, max_ticks, dif_ticks;
	#endif /* DEBUG_INST */

	monorale_hdr *hdr = (monorale_hdr*)arg;
	u32 frame = 0;
	
	u16 *fb;

	#ifdef DEBUGINST
	tot_ticks = 0;
	min_ticks = ~0ULL;
	max_ticks = 0;
	#endif /* DEBUG_INST */

	//printf("LFB:%08u RFB:%08u",&lfb,&rfb);
	while(runVideo && frame < monorale_frames(hdr)) {
		gspWaitForVBlank();
		//gspWaitForVBlank();

		while(runVideo && !playVideo)
			svcSleepThread(10e9 / 60);
	
		#ifdef DEBUGINST
		dif_ticks = svcGetSystemTick();
		#endif

		fb = (u16*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		monorale_doframe(hdr, frame, fb);
		
		#ifdef DEBUGINST
		dif_ticks = svcGetSystemTick() - dif_ticks;
		if(dif_ticks >= (SYSCLOCK_ARM11_NEW/1000.0)) {
			printf("frame:%ld, over 1ms.",frame);
		}
		min_ticks = (dif_ticks < min_ticks) ? dif_ticks : min_ticks;
		max_ticks = (dif_ticks > max_ticks) ? dif_ticks : max_ticks;
		tot_ticks += dif_ticks;
		#endif /* DEBUG_INST */

		gfxFlushBuffers();
		gfxSwapBuffers();
		frame++;
	}

	runVideo = 0;
	#ifdef DEBUGINST
	printf("frames = %ld, ticks = %llu, min_ticks = %llu,"
			"max_ticks = %llu, avg_ticks = %llu\n",
			frame, tot_ticks, min_ticks, max_ticks,
			tot_ticks / frame);

	printf("min_ticks = %f ms, max_ticks = %f ms, avg_ticks = %f ms\n",
			(double)min_ticks / (SYSCLOCK_ARM11_NEW/1000.0),
			(double)max_ticks / (SYSCLOCK_ARM11_NEW/1000.0),
			(double)(tot_ticks / frame) / (SYSCLOCK_ARM11_NEW/1000.0));
	#endif /* DEBUG_INST */	
	threadExit(0);
}
