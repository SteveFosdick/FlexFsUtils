#include "flexfsu.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct flexdisc *ffu_open(const char *fn)
{
    FILE *fp = fopen(fn, "rb");
    if (fp) {
        struct flexdisc *disc = malloc(sizeof(struct flexdisc));
        if (disc) {
            if (fseek(fp, 2 * FLEX_SECT_SIZE, SEEK_SET) == 0) {
                if (fread(disc->fdi_sect, FLEX_SECT_SIZE, 1, fp) == 1) {
                    disc->fdi_fp = fp;
                    disc->fdi_max_track  = disc->fdi_sect[0x26];
                    disc->fdi_max_sector = disc->fdi_sect[0x27];
                    return disc;
                }
            }
            free(disc);
        }
        fclose(fp);
    }
    return NULL;
}

void ffu_close(struct flexdisc *disc)
{
    fclose(disc->fdi_fp);
    free(disc);
}

const unsigned char *ffu_read_sect(struct flexdisc *disc, int track, int sector)
{
    if (track > disc->fdi_max_track || sector > disc->fdi_max_sector) {
        errno = EINVAL;
        return NULL;
    }
    if (fseek(disc->fdi_fp, (track * disc->fdi_max_sector + sector - 1) * FLEX_SECT_SIZE, SEEK_SET))
        return NULL;
    if (fread(disc->fdi_sect, FLEX_SECT_SIZE, 1, disc->fdi_fp) != 1)
        return NULL;
    return disc->fdi_sect;
}

static const unsigned char *next_dir_ent(struct flexdisc *disc, const unsigned char *dirptr)
{
    const unsigned char *sect;
    unsigned n;

    do {
        dirptr += 24;
        if (dirptr >= disc->fdi_sect + FLEX_SECT_SIZE) {
            int track = disc->fdi_sect[0];
            int sector = disc->fdi_sect[1];
            if (track == 0 && sector == 0)
                return NULL;
            sect = ffu_read_sect(disc, track, sector);
            if (!sect)
                return NULL;
            dirptr = sect + 16;
        }
        n = *dirptr;
    }
    while (!n || (n & 0x80));
    disc->fdi_dirptr = dirptr;
    return dirptr;
}

const unsigned char *ffu_dir_first(struct flexdisc *disc)
{
    if (fseek(disc->fdi_fp, 4 * FLEX_SECT_SIZE, SEEK_SET))
        return NULL;
    if (fread(disc->fdi_sect, FLEX_SECT_SIZE, 1, disc->fdi_fp) != 1)
        return NULL;
    return next_dir_ent(disc, disc->fdi_sect + 16 - 24);
}

const unsigned char *ffu_dir_next(struct flexdisc *disc)
{
    return next_dir_ent(disc, disc->fdi_dirptr);
}

const unsigned char *ffu_find_file(struct flexdisc *disc, const char *fn)
{
    char ch, *ptr, *end, pat[11];
    const unsigned char *dirptr;

    const char *ext = strchr(fn, '.');
    if (!ext || (ext - fn) > 8 || strlen(ext) > 4)
        return 0;
    ptr = pat;
    end = ptr + 8;
    while (fn < ext && ptr < end)
        *ptr++ = *fn++ & 0xdf;
    while (ptr < end)
        *ptr++ = 0;
    end += 3;
    while ((ch = *++ext) && ptr < end)
        *ptr++ = ch  & 0xdf;
    while (ptr < end)
        *ptr++ = 0;

    dirptr = ffu_dir_first(disc);
    while (dirptr) {
        if (memcmp(dirptr, pat, 11) == 0)
            return dirptr;
        dirptr = next_dir_ent(disc, disc->fdi_dirptr);
    }
    return NULL;
}
