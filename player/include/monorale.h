#pragma once

#include <3ds.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern volatile u32 runVideo;

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

monorale_hdr *monorale_init(const char *path);
void monoraleThread(void *arg);
