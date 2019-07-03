#include "flexfsu.h"

int main(int argc, char **argv)
{
    int status;

    if (argc == 2) {
        const char *fn = argv[1];
        struct flexdisc *disc = ffu_open(fn);
        if (disc) {
            unsigned freesect = (disc->fdi_sir[17] << 8) | disc->fdi_sir[18];
            const unsigned char *di = ffu_dir_first(disc);
            printf("Volume %.10s number %d created %02d/%02d/%04d\n",
                   disc->fdi_sir, (disc->fdi_sir[11] << 8) | disc->fdi_sir[12],
                   disc->fdi_sir[20], disc->fdi_sir[19], disc->fdi_sir[21] + 1900);
            status = 0;
            while (di) {
                unsigned attr = di[11];
                printf("%-8.8s %-3.3s %c%c%c%c %2d:%02d %2d:%02d %5d %c %02d/%02d/%04d\n",
                       di, di+8,
                       attr & 0x80 ? 'W' : ' ', attr & 0x40 ? 'D' : ' ',
                       attr & 0x20 ? 'R' : ' ', attr & 0x20 ? 'C' : ' ',
                       di[13], di[14], di[15], di[16],
                       (di[17] << 8) | di[18], di[19] ? 'R' : ' ',
                       di[22], di[21], di[23] + 1900);
                di = ffu_dir_next(disc);
            }
            ffu_close(disc);
            printf("%u free sectors\n", freesect);
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
