#pragma once

#include <3ds.h>

typedef struct {
	uint32_t offset;
	uint32_t cmdcnt;
} __attribute__((packed)) monorale_frameinf;

typedef struct {
	uint32_t framecnt;
	monorale_frameinf inf[0];
} __attribute__((packed)) monorale_hdr;

static inline size_t monorale_frames(monorale_hdr *hdr) {
	return hdr->framecnt;
}

void monorale_doframe(monorale_hdr *hdr, size_t frame, u16 *fb);
