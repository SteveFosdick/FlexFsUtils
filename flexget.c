#include "flexfsu.h"
#include <errno.h>
#include <string.h>

static int binary_content(const unsigned char *data, FILE *ofp, int flag)
{
    fwrite(data, FLEX_SECT_SIZE-4, 1, ofp);
    return flag;
}

static int text_content(const unsigned char *data, FILE *ofp, int tab)
{
    const unsigned char *end = data + FLEX_SECT_SIZE-4;
    while (data < end) {
        unsigned char ch = *data++;
        if (tab) {
            tab = *data++;
            while (tab--)
                putc(' ', ofp);
        }
        else {
            switch(ch) {
                case '\t':
                    tab = 1;
                    break;
                case '\r':
                    ch = '\n';
                    /* fallthrough */
                default:
                    putc(ch, ofp);
                case 0:
                case 0x18:
                    break;
            }
        }
    }
    return tab;
}

static int extract_file(struct flexdisc *disc, const char *fn, const char *mode, int (*callback)(const unsigned char *data, FILE *ofp, int flag))
{
    int status;
    char pat[FLEX_NAME_SIZE];

    if (ffu_parse_fn(fn, pat)) {
        int track = 0;
        int sector = 5;
        do {
            const unsigned char *sect = ffu_read_sect(disc, track, sector);
            if (!sect) {
                fprintf(stderr, "flexget: error reading sector %d:%d: %s\n", track, sector, strerror(errno));
                status = 3;
                break;
            }
            const unsigned char *ptr = sect+16;
            const unsigned char *end = sect+FLEX_SECT_SIZE;
            while (ptr < end) {
                unsigned n = *ptr;
                if (n && !(n & 0x80) && memcmp(ptr, pat, FLEX_NAME_SIZE) == 0) {
                    FILE *ofp = fopen(fn, mode);
                    if (ofp) {
                        int flag = 0;
                        unsigned track = ptr[13];
                        unsigned sector = ptr[14];
                        while (track && sector) {
                            const unsigned char *data = ffu_read_sect(disc, track, sector);
                            if (!data) {
                                fprintf(stderr, "flexget: error reading sector %d:%d of %s: %s\n", track, sector, fn, strerror(errno));
                                status = 5;
                                break;
                            }
                            flag = callback(data+4, ofp, flag);
                            track  = data[0];
                            sector = data[1];
                        }
                        fclose(ofp);
                        status = 0;
                    }
                    else {
                        fprintf(stderr, "flexget: unable to open %s for writing: %s\n", fn, strerror(errno));
                        status = 4;
                    }
                    return status;
                }
                ptr += 24;
            }
            track = sect[0];
            sector = sect[1];
        }
        while (track || sector);
        fprintf(stderr, "flexget: file %s not found on flex disc\n", fn);
        status = 3;
    }
    else {
        fprintf(stderr, "flexget: illegal filename %s\n", fn);
        status = 3;
    }
    return status;
}

enum filetype {
    FT_UNKNOWN,
    FT_BINARY,
    FT_TEXT
};

int main(int argc, char **argv)
{
    int status = 0;

    if (argc >= 4) {
        enum filetype file_type = FT_UNKNOWN;
        struct flexdisc *disc = NULL;
        while (--argc) {
            const char *arg = *++argv;
            if (arg[0] == '-') {
                if (arg[1] == 't')
                    file_type = FT_TEXT;
                else if (arg[1] == 'b')
                    file_type = FT_BINARY;
                else {
                    fprintf(stderr, "flexget: invalid option '%c'\n", arg[1]);
                    status = 1;
                    break;
                }
            }
            else if (!disc) {
                if (!(disc = ffu_open(arg))) {
                    fprintf(stderr, "flexget: unable to open flex disc '%s': %m\n", arg);
                    status = 2;
                    break;
                }
            }
            else if (file_type == FT_BINARY)
                status = extract_file(disc, arg, "wb", binary_content);
            else if (file_type == FT_TEXT)
                status = extract_file(disc, arg, "w", text_content);
            else {
                fputs("flexget: file type (-b/-t) must be specified\n", stderr);
                status = 1;
            }
        }
    }
    else {
        fputs("Usage: flexget -b|-t <disc-img> <flex-file> [ <flex-file> ... ]\n", stderr);
        status = 1;
    }
    return status;
}
