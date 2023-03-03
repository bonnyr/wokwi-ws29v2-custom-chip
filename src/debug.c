#include "debug.h"
#include "wokwi-api.h"

char buf[400];

const char *debugHexStr(uint8_t *p, size_t c) {
    char *pb = (char *)buf;
    c = constrain(c, 0, 128);
    for (; c--;) {
        pb += sprintf(pb, "%02x ", *p++);
    }
    *pb = 0;
    return buf;
}


void debugBuffer(const char *m, uint8_t *p, size_t l, size_t w) {
    printf("%lld %s\n", get_sim_nanos() / 1000, m); 

    w = min(w, 128);

    int r = 0;
    while (l > 0) {
        printf(" %4d  %s\n",r, debugHexStr(p, min(w, l)));
        if (l < w) {
            break;
        }
        l -= w;
        p += w;
        r++;
    }
}
