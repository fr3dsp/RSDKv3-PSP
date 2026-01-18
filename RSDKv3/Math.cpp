#include "RetroEngine.hpp"
#include <math.h>
#include <time.h>

#if !RETRO_USING_PSP
int sinMLookupTable[512];
int cosMLookupTable[512];

int sin512LookupTable[512];
int cos512LookupTable[512];

int sin256LookupTable[256];
int cos256LookupTable[256];

byte arcTan256LookupTable[0x100 * 0x100];
#endif

void CalculateTrigAngles()
{
    srand((unsigned)time(NULL));

#if !RETRO_USING_PSP
    for (int i = 0; i < 0x200; ++i) {
        sinMLookupTable[i] = (sin((i / 256.0) * RSDK_PI) * 4096.0);
        cosMLookupTable[i] = (cos((i / 256.0) * RSDK_PI) * 4096.0);
    }

    cosMLookupTable[0x00]  = 0x1000;
    cosMLookupTable[0x80]  = 0;
    cosMLookupTable[0x100] = -0x1000;
    cosMLookupTable[0x180] = 0;

    sinMLookupTable[0x00]  = 0;
    sinMLookupTable[0x80]  = 0x1000;
    sinMLookupTable[0x100] = 0;
    sinMLookupTable[0x180] = -0x1000;

    for (int i = 0; i < 0x200; ++i) {
        sin512LookupTable[i] = (sinf((i / 256.0) * RSDK_PI) * 512.0);
        cos512LookupTable[i] = (cosf((i / 256.0) * RSDK_PI) * 512.0);
    }

    cos512LookupTable[0x00]  = 0x200;
    cos512LookupTable[0x80]  = 0;
    cos512LookupTable[0x100] = -0x200;
    cos512LookupTable[0x180] = 0;

    sin512LookupTable[0x00]  = 0;
    sin512LookupTable[0x80]  = 0x200;
    sin512LookupTable[0x100] = 0;
    sin512LookupTable[0x180] = -0x200;

    for (int i = 0; i < 0x100; i++) {
        sin256LookupTable[i] = (sin512LookupTable[i * 2] >> 1);
        cos256LookupTable[i] = (cos512LookupTable[i * 2] >> 1);
    }

    for (int Y = 0; Y < 0x100; ++Y) {
        byte *atan = (byte *)&arcTan256LookupTable[Y];
        for (int X = 0; X < 0x100; ++X) {
            float angle = atan2f(Y, X);
            *atan       = (angle * 40.743664f);
            atan += 0x100;
        }
    }
#endif
}

byte ArcTanLookup(int X, int Y)
{
    int x = abs(X);
    int y = abs(Y);

    if (x <= y) {
        while (y > 0xFF) {
            x >>= 4;
            y >>= 4;
        }
    }
    else {
        while (x > 0xFF) {
            x >>= 4;
            y >>= 4;
        }
    }

#if RETRO_USING_PSP
    if (x == 0 && y == 0)
        return 0;
    
    float atanVal = atan2f((float)y, (float)x) * 40.743664f;
    if (!isfinite(atanVal))
        return 0;
    
    int result = (int)atanVal;
    if (X <= 0) {
        if (Y <= 0)
            return result + 0x80;
        else
            return 0x80 - result;
    }
    else if (Y <= 0)
        return -result;
    else
        return result;
#else
    if (X <= 0) {
        if (Y <= 0)
            return arcTan256LookupTable[(x << 8) + y] + -0x80;
        else
            return -0x80 - arcTan256LookupTable[(x << 8) + y];
    }
    else if (Y <= 0)
        return -arcTan256LookupTable[(x << 8) + y];
    else
        return arcTan256LookupTable[(x << 8) + y];
#endif
}
