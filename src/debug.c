#include "debug.h"


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
