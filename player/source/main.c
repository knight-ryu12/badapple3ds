#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <3ds.h>

#define DEBUG_INST /* minor profiling */

typedef struct {
	uint32_t offset;
	uint32_t cmdcnt;
} __attribute__((packed)) monorle_frameinf;

typedef struct {
	uint32_t framecnt;
	monorle_frameinf inf[0];
} __attribute__((packed)) monorle_hdr;

static inline u16 *monorle_frame(monorle_hdr *hdr, size_t frame)
{
	monorle_frameinf *info;
	if (frame >= hdr->framecnt) return NULL;

	info = &hdr->inf[frame];
	return (u16*)((char*)hdr + info->offset);
}

static inline uint32_t monorle_framecnt(monorle_hdr *hdr, size_t frame)
{
	if (frame >= hdr->framecnt) return 0;
	return hdr->inf[frame].cmdcnt;
}

static void monorle_doframe(monorle_hdr *hdr, size_t frame, u16 *fb)
{
	u32 fill_val = 0;
	u16 *cmds = monorle_frame(hdr, frame);
	size_t cmdcnt = monorle_framecnt(hdr, frame);

	for (size_t i = 0; i < cmdcnt; i++) {
		size_t len_px, len_b;

		len_px = cmds[i];
		len_b = cmds[i] << 1;

		memset(fb, fill_val, len_b);

		fb += len_px;
		fill_val ^= 0xFFFFFFFF;
	}
}

int main(int argc, char **argv)
{
	Result res;
	FILE *vid;
	size_t vid_sz, frame;
	monorle_hdr *vid_hdr;

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

	printf("Starting...\n");
	fread(vid_hdr, vid_sz, 1, vid);
	fclose(vid);

	frame = 0;

	#ifdef DEBUG_INST
	tot_ticks = 0;
	min_ticks = ~0ULL;
	max_ticks = 0;
	#endif /* DEBUG_INST */

	while(aptMainLoop() && (frame < vid_hdr->framecnt)) {
		u16 *fb;

		gspWaitForVBlank();

		hidScanInput();
		if (hidKeysDown() & KEY_B) break;

		#ifdef DEBUG_INST
		dif_ticks = svcGetSystemTick();
		#endif /* DEBUG_INST */

		fb = (u16*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		monorle_doframe(vid_hdr, frame, fb);

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
			tot_ticks / vid_hdr->framecnt);

	printf("min_ticks = %f ms, max_ticks = %f ms, avg_ticks = %f ms\n",
			(double)min_ticks / CPU_TICKS_PER_MSEC,
			(double)max_ticks / CPU_TICKS_PER_MSEC,
			(double)(tot_ticks / vid_hdr->framecnt) / CPU_TICKS_PER_MSEC);
	#endif /* DEBUG_INST */

	free(vid_hdr);

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();
		if (hidKeysDown() & KEY_START)
			break; // break in order to return to hbmenu
	}

	gfxExit();
	return 0;
}
