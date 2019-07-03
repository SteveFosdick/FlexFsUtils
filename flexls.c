#include "flexfsu.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct flex_di flexdi;

struct flex_di {
    flexdi *next;
    unsigned char di[24];
};

static flexdi *sort_di(flexdi *list)
{
    flexdi *p, *q, *e, *tail;
    int insize, nmerges, psize, qsize;

    insize = 1;
    while (1) {
        p = list;
        list = tail = NULL;
        nmerges = 0;

        while (p) {
            nmerges++; /* there exists a merge to be done */
            q = p;
            /* step `insize' places along from p */
            psize = 0;
            do {
                if (psize >= insize)
                    break;
                psize++;
                q = q->next;
            } while (q);
            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;

            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) {
                /* decide whether next element of merge comes from p or q */
                if (psize == 0) {
                    /* p is empty; e must come from q. */
                    e = q;
                    q = q->next;
                    qsize--;
                }
                else if (qsize == 0 || !q) {
                    /* q is empty; e must come from p. */
                    e = p;
                    p = p->next;
                    psize--;
                }
                else if (memcmp(p->di, q->di, 11) <= 0) {
                    /* First element of p is lower (or same);
                    * e must come from p. */
                    e = p;
                    p = p->next;
                    psize--;
                }
                else {
                    /* First element of q is lower; e must come from q. */
                    e = q;
                    q = q->next;
                    qsize--;
                }
                /* add the next element to the merged list */
                if (tail)
                    tail->next = e;
                else
                    list = e;
                tail = e;
            }
            /* now p has stepped `insize' places along, and q has too */
            p = q;
        }
        tail->next = NULL;

        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1) /* allow for nmerges==0, the empty list case */
            return list;

        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
}


int main(int argc, char **argv)
{
    int status;

    if (argc == 2) {
        const char *fn = argv[1];
        struct flexdisc *disc = ffu_open(fn);
        if (disc) {
            printf("Volume %.10s number %d created %02d/%02d/%04d\n",
                   disc->fdi_sir, (disc->fdi_sir[11] << 8) | disc->fdi_sir[12],
                   disc->fdi_sir[20], disc->fdi_sir[19], disc->fdi_sir[21] + 1900);
            flexdi *list = NULL;
            int track = 0;
            int sector = 5;
            do {
                const unsigned char *sect = ffu_read_sect(disc, track, sector);
                if (!sect) {
                    fprintf(stderr, "flexls: error reading sector %d:%d from %s: %s\n", track, sector, fn, strerror(errno));
                    status = 3;
                    break;
                }
                const unsigned char *ptr = sect+16;
                const unsigned char *end = sect+FLEX_SECT_SIZE;
                while (ptr < end) {
                    unsigned n = *ptr;
                    if (n && !(n & 0x80)) {
                        flexdi *ent = malloc(sizeof(flexdi));
                        if (!ent) {
                            fputs("flexls: out of memory\n", stderr);
                            return 2;
                        }
                        memcpy(ent->di, ptr, sizeof ent->di);
                        ent->next = list;
                        list = ent;
                    }
                    ptr += 24;
                }
                track = sect[0];
                sector = sect[1];
            }
            while (track || sector);

            if (list) {
                list = sort_di(list);

                do {
                    unsigned attr = list->di[11];
                    printf("%-8.8s %-3.3s %c%c%c%c %2d:%02d %2d:%02d %5d %c %02d/%02d/%04d\n",
                           list->di, list->di+8,
                           attr & 0x80 ? 'W' : ' ', attr & 0x40 ? 'D' : ' ',
                           attr & 0x20 ? 'R' : ' ', attr & 0x20 ? 'C' : ' ',
                           list->di[13], list->di[14], list->di[15], list->di[16],
                           (list->di[17] << 8) | list->di[18], list->di[19] ? 'R' : ' ',
                           list->di[22], list->di[21], list->di[23] + 1900);
                    flexdi *next = list->next;
                    free(list);
                    list = next;
                }
                while (list);
            }
            printf("%u free sectors\n", (disc->fdi_sir[17] << 8) | disc->fdi_sir[18]);
            ffu_close(disc);
        }
        else {
            fprintf(stderr, "flexls: unable to open flex disc '%s': %m\n", fn);
            status = 2;
        }
    }
    else {
        fputs("Usage: flexls <disc-img>\n", stderr);
        status = 1;
    }
    return status;
}
