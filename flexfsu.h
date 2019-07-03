#ifndef FLEX_FSUTILS_INC
#define FLEX_FSUTILS_INC

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define FLEX_SECT_SIZE 256
#define FLEX_NAME_SIZE  11

struct flexdisc {
    FILE *fdi_fp;
    unsigned char fdi_sir[24];
    unsigned char fdi_sect[FLEX_SECT_SIZE];
};

struct flexdisc *ffu_open(const char *fn);
void ffu_close(struct flexdisc *disc);
const unsigned char *ffu_read_sect(struct flexdisc *disc, int track, int sector);
bool ffu_parse_fn(const char *fn, unsigned char *buf);

#endif
