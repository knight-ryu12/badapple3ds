#include <3ds.h>
#include <string.h>

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
