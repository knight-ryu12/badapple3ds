#pragma once

#include <3ds.h>

#include <string.h>

#include <tremor/ivorbiscodec.h>
#include <tremor/ivorbisfile.h>

extern volatile u32 runSound, playSound;

Result initSound(u16 sample_rate);

int initVorbis(const char *path, OggVorbis_File *vorbisFile, vorbis_info **vi);
void soundThread(void *arg);
