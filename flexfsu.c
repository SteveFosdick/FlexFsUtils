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
            if (fseek(fp, 2 * FLEX_SECT_SIZE + 16, SEEK_SET) == 0) {
                if (fread(disc->fdi_sir, sizeof disc->fdi_sir, 1, fp) == 1) {
                    disc->fdi_fp = fp;
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
    unsigned max_track  = disc->fdi_sir[22];
    unsigned max_sector = disc->fdi_sir[23];
    if (track > max_track || sector > max_sector) {
        errno = EINVAL;
        return NULL;
    }
    if (fseek(disc->fdi_fp, (track * max_sector + sector - 1) * FLEX_SECT_SIZE, SEEK_SET))
        return NULL;
    if (fread(disc->fdi_sect, FLEX_SECT_SIZE, 1, disc->fdi_fp) != 1)
        return NULL;
    return disc->fdi_sect;
}

bool ffu_parse_fn(const char *fn, char *buf)
{
    char ch, *ptr, *end;
    const char *ext = strchr(fn, '.');
    if (!ext || (ext - fn) > 8 || strlen(ext) > 4)
        return false;
    ptr = buf;
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
    return true;
}
