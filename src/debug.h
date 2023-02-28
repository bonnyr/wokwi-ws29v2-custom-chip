// --------------- Debug Macros -----------------------
#ifndef __CUSTOM_CHIP_DEBUG_H__
#define __CUSTOM_CHIP_DEBUG_H__


#include <stdlib.h>
#include <stdio.h>


#define max(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = b; _a > _b ? _a : _b; })
#define min(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = b; _a < _b ? _a : _b; })
#define constrain(v, n, x) ({__typeof__(v) _v = (v); __typeof__(n) _n = (n); __typeof__(x) _x = x; min(max(_v,_n),_x); })
#define in_range(v, l, r) ({__typeof__(v) _v = (v); __typeof__(l) _l = (l); __typeof__(r) _r = r; _l <= _v && _v <= _r; })

// --------------- Debug Macros -----------------------
#ifdef DEBUG
#define DBGF_SPI 0x01
#define DBGF_I2C 0x02
#define DBGF_TMR 0x04
#define DBGF_GEN 0x80

#define DBG_SPI_PFX "(spi): "
#define DBG_I2C_PFX "(i2c): "
#define DBG_TMR_PFX "(tmr): "
#define DBG_GEN_PFX "(gen): "

#define DEBUGF(...)                                  \
    {                                                \
        if (chip->debug) {                           \
            printf("%lld ", get_sim_nanos() / 1000); \
            printf(__VA_ARGS__);                     \
        }                                            \
    }

#define DEBUGF_PFX(mask, pfx, ...)                   \
    {                                                \
        if (chip->debug && (chip->debug_mask & mask)) {                           \
            printf("%lld %s", get_sim_nanos() / 1000, pfx); \
            printf(__VA_ARGS__);                     \
        }                                            \
    }


#define SPI_DEBUGF(...)   DEBUGF_PFX(DBGF_SPI, DBG_SPI_PFX, __VA_ARGS__);
#define I2C_DEBUGF(...)   DEBUGF_PFX(DBGF_I2C, DBG_I2C_PFX, __VA_ARGS__);
#define TMR_DEBUGF(...)   DEBUGF_PFX(DBGF_TMR, DBG_TMR_PFX, __VA_ARGS__);
#define GEN_DEBUGF(...)   DEBUGF_PFX(DBGF_GEN, DBG_GEN_PFX, __VA_ARGS__);



extern char buf[400];
extern const char *debugHexStr(uint8_t *p, size_t c);

#else
    #define SPI_DEBUGF(...)   
    #define I2C_DEBUGF(...)   
    #define TMR_DEBUGF(...)   
    #define GEN_DEBUGF(...)   

#endif  //  DEBUG

#endif //  __CUSTOM_CHIP_DEBUG_H__
// --------------- Debug Macros -----------------------

