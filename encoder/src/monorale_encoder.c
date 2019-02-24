/* compile with `cc -O2 monorale_encoder.c -o monorale_encoder` */

#include <assert.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#define GREY_THRESHOLD	(33808) /* grey */

#define SCREEN_SZ		(400*240)

typedef uint16_t rle_t;

#define RLE_MAX (1ULL << (sizeof(rle_t) * 8))

typedef struct {
	uint32_t offset;
	uint32_t cmdcnt;
} __attribute__((packed)) monorle_frameinf;

typedef struct {
	uint32_t framecnt;
	monorle_frameinf inf[0];
} __attribute__((packed)) monorle_hdr;

/** THIS CODE IS FOR THE PLAYER
static inline rle_t *monorle_frame(monorle_hdr *hdr, size_t frame)
{
	monorle_frameinf *info;
	if (frame >= hdr->framecnt) return NULL;

	info = &hdr->inf[frame];
	return (rle_t*)((char*)hdr + info->offset);
}

static inline uint32_t monorle_framecnt(monorle_hdr *hdr, size_t frame)
{
	if (frame >= hdr->framecnt) return 0;
	return hdr->inf[frame].cmdcnt;
} **/

const struct option longOptions[] = {
    /* clang-format off */
	{ "help",      no_argument,       NULL, 'h', },
	{ "output",    required_argument, NULL, 'o', },
	{ "version",   no_argument,       NULL, 'v', },
    /* clang-format on */
};

void printUsage(void) {
	printf("Monorale encoder, by Wolfvak and Chromaryu.\n");
	printf("Monorale encoder encodes rgb565 video files into monorale and outputs as monorale.bin.\n");
	printf("Usage: monorale_encoder(.exe) <Options> <Input>\n");
	printf("Options:\n");
	for(int optIndex = 0; optIndex < sizeof(longOptions) / sizeof(struct option); optIndex++){
		printf("  -%c, --%s, %s\n", longOptions[optIndex].val, longOptions[optIndex].name,
			 longOptions[optIndex].has_arg == no_argument ? "" :
				 longOptions[optIndex].has_arg == required_argument ? "<arg> (required)" : "<arg> (optional)");
	}
}

int encode_monorle(const char* input, const char* output) {
	size_t vidsz, framecnt, hdr_sz;
	monorle_hdr *hdr;
	FILE *vid, *out;

	assert(input);
	assert(output);
	vid = fopen(input, "rb");
	out = fopen(output, "wb+");
	uint16_t *vidbuf = malloc(SCREEN_SZ * 2);
	assert(vid != NULL && out != NULL && vidbuf != NULL);

	fseek(vid, 0L, SEEK_END);
	vidsz = ftell(vid);
	fseek(vid, 0L, SEEK_SET);

	framecnt = vidsz / (SCREEN_SZ * 2);

	hdr_sz = sizeof(monorle_hdr) + (sizeof(monorle_frameinf) * framecnt);
	hdr = malloc(hdr_sz);
	assert(hdr != NULL);

	fseek(out, hdr_sz, SEEK_SET);

	for (size_t i = 0; i < framecnt; i++) {
		size_t vidpx = 0, rlecnt = 0;

		hdr->inf[i].offset = ftell(out);

		fread(vidbuf, SCREEN_SZ * 2, 1, vid);

		while(1) {
			/*
			 * rle: the pixel count to be set
			 * vidpx: the current pixel index
			 */

			rle_t rle = 0;
			while((vidbuf[vidpx] < GREY_THRESHOLD) && (vidpx < SCREEN_SZ)) {
				rle++;
				vidpx++;
				if (rle == RLE_MAX-1) {
				/*
				 * about to overflow - break out of the loop
				 * and write the current rle count.
				 * if there were more pixels of the same color, the next
				 * loop will just write zero pixels to be set and come back
				 * here.
				 */
					break;
				}
			}
			fwrite(&rle, sizeof(rle_t), 1, out);
			rlecnt++;

			if (vidpx == SCREEN_SZ) break;

			rle = 0;
			while(vidbuf[vidpx] >= GREY_THRESHOLD && (vidpx < SCREEN_SZ)) {
				rle++;
				vidpx++;
				if (rle == RLE_MAX-1) break;
			}
			fwrite(&rle, sizeof(rle_t), 1, out);
			rlecnt++;

			if (vidpx == SCREEN_SZ) break;
		}

		hdr->inf[i].cmdcnt = rlecnt;

		printf("frame %d/%d, %d KiB\r", i, framecnt, ftell(out) >> 10);
	}

	hdr->framecnt = framecnt;
	fseek(out, 0L, SEEK_SET);
	fwrite(hdr, hdr_sz, 1, out);

	fclose(out);
	fclose(vid);
	free(hdr);
	free(vidbuf);
	printf("\n");
}

int main(int argc, char *argv[])
{
	int argi = 1;
	int argci;
	char *outputPath = NULL;

	if(argc == 1){
		/* no arguments */
		printUsage();
		return EXIT_SUCCESS;
	}
	else{
		// parse options
		int c;
		while ((c = getopt_long(argc, argv, "ho:v:", longOptions, NULL)) != -1)
		{
			switch (c) {
				case 'h':
					// show help
					printUsage();
					return EXIT_SUCCESS;

				case 'o':
					// set output path option
					outputPath = optarg;
					continue;

				case 'v':
					// print version
					//printVersion();
					return EXIT_SUCCESS;
			}
		}
	}

	if (optind >= argc)
	{
		printf("No input file provided\n");
		return EXIT_FAILURE;
	}

	if (outputPath == NULL) {
		printf("No output file provided\n");
		return EXIT_FAILURE;
	}

	/* input files */
	while(optind < argc) {
		encode_monorle(argv[optind++], outputPath);
	}
	return 0;
}
