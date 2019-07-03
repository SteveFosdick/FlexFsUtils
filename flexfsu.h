#ifndef FLEX_FSUTILS_INC
#define FLEX_FSUTILS_INC

#include <stdint.h>
#include <stdio.h>

#define FLEX_SECT_SIZE 256

struct flexdisc {
    FILE *fdi_fp;
    const unsigned char *fdi_dirptr;
    unsigned fdi_max_track;
    unsigned fdi_max_sector;
    unsigned char fdi_sect[FLEX_SECT_SIZE];
};

struct flexdisc *ffu_open(const char *fn);
void ffu_close(struct flexdisc *disc);
const unsigned char *ffu_read_sect(struct flexdisc *disc, int track, int sector);
const unsigned char *ffu_dir_first(struct flexdisc *disc);
const unsigned char *ffu_dir_next(struct flexdisc *disc);
const unsigned char *ffu_find_file(struct flexdisc *disc, const char *fn);

#endif
