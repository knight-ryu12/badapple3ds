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
	monorale_hdr *hdr = (monorale_hdr*)arg;
	u32 frame = 0;

	while(runVideo && frame < monorale_frames(hdr)) {
		gspWaitForVBlank();
		gspWaitForVBlank();

		u16 *fb = (u16*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		monorale_doframe(hdr, frame, fb);

		gfxFlushBuffers();
		gfxSwapBuffers();
		frame++;
	}

	runVideo = 0;
	threadExit(0);
}
