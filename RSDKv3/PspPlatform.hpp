/*
 * Since I genuinely feel like being a dick, for anyone else doing a PSP port of the other RSDK versions you may **NOT** use this file without my permission (yes that includes you Freddy).
 * If you WANT to use it email: funkiin.forever@gmail.com
 *  - ven <3
*/
#ifndef PSP_PLATFORM_H
#define PSP_PLATFORM_H

#if RETRO_PLATFORM == RETRO_PSP

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspge.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psprtc.h>
#include <pspctrl.h>
#include <psppower.h>
#include <pspdebug.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define PSP_SCREEN_WIDTH  480
#define PSP_SCREEN_HEIGHT 272
#define PSP_LINE_SIZE     512
#define PSP_PIXEL_FORMAT  GU_PSM_5650

#define PSP_VRAM_BASE     0x04000000
#define PSP_VRAM_WIDTH    512
#define PSP_VRAM_HEIGHT   272
#define PSP_VRAM_BUFSIZE  (PSP_VRAM_WIDTH * PSP_VRAM_HEIGHT * 2)

extern ushort* pspVramBuffer;
extern u64 pspTickFrequency;
extern volatile bool pspAudioRunning;
extern void (*pspUserAudioCallback)(void*, unsigned int, void*);
extern void* pspUserAudioData;

extern unsigned int pspDisplayList[256];

namespace PspPlatform {

inline void DebugLog(const char* fmt, ...) {
    static FILE* logFile = nullptr;
    static bool initialized = false;
    static int lineCount = 0;
    
    if (!initialized) {
        logFile = fopen("ms0:/psp_debug.log", "w");
        initialized = true;
    }
    
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    if (logFile) {
        fprintf(logFile, "%s\n", buffer);
        fflush(logFile);
    }
    
    if (lineCount < 30) {
        pspDebugScreenPrintf("%s\n", buffer);
        lineCount++;
    }
}

inline void InitDebugScreen() {
    pspDebugScreenInit();
    pspDebugScreenSetXY(0, 0);
    DebugLog("=== PSP Debug Log ===");
}

inline int InitDisplay() {
    pspVramBuffer = (ushort*)0x44000000;
    
    sceGuInit();
    sceGuStart(GU_DIRECT, pspDisplayList);
    sceGuDrawBuffer(GU_PSM_5650, (void*)0, PSP_LINE_SIZE);
    sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, (void*)0, PSP_LINE_SIZE);
    sceGuFinish();
    sceGuSync(0, 0);
    
    sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
    sceDisplayWaitVblankStart();
    sceDisplaySetFrameBuf(pspVramBuffer, PSP_LINE_SIZE, PSP_DISPLAY_PIXEL_FORMAT_565, PSP_DISPLAY_SETBUF_NEXTFRAME);
    sceGuDisplay(GU_TRUE);
    
    memset(pspVramBuffer, 0, PSP_VRAM_BUFSIZE);
    
    return 1;
}

inline void ReleaseDisplay() {
}

inline void WaitVblank() {
    sceDisplayWaitVblankStart();
}

inline void FlipScreen() {
    sceKernelDcacheWritebackAll();
}

inline void CopyToVram(ushort* src, int srcWidth, int srcHeight, int srcPitch) {
    ushort* vram = pspVramBuffer;
    
    int offsetX = (PSP_SCREEN_WIDTH - srcWidth) / 2;
    int offsetY = (PSP_SCREEN_HEIGHT - srcHeight) / 2;
    
    if (srcPitch == srcWidth && srcWidth == PSP_SCREEN_WIDTH && offsetX == 0) {
        ushort* dst = vram + offsetY * PSP_LINE_SIZE;
        for (int y = 0; y < srcHeight; y++) {
            memcpy(dst, src, srcWidth * 2);
            src += srcPitch;
            dst += PSP_LINE_SIZE;
        }
    } else {
        vram += offsetY * PSP_LINE_SIZE + offsetX;
        for (int y = 0; y < srcHeight; y++) {
            memcpy(vram, src, srcWidth * 2);
            src += srcPitch;
            vram += PSP_LINE_SIZE;
        }
    }
}

inline void ClearScreen(ushort color) {
    ushort* vram = pspVramBuffer;
    for (int i = 0; i < PSP_LINE_SIZE * PSP_SCREEN_HEIGHT; i++) {
        vram[i] = color;
    }
}

inline void InitTiming() {
    pspTickFrequency = sceRtcGetTickResolution();
}

inline u64 GetPerformanceCounter() {
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    return tick;
}

inline u64 GetPerformanceFrequency() {
    return pspTickFrequency;
}

typedef void (*PspAudioCallback)(void* buffer, unsigned int samples, void* userdata);

inline void AudioThread(void* buf, unsigned int reqn, void* userdata) {
    if (pspUserAudioCallback && pspAudioRunning) {
        pspUserAudioCallback(buf, reqn, pspUserAudioData);
    } else {
        memset(buf, 0, reqn * 4);
    }
}

inline int InitAudio(PspAudioCallback callback, void* userdata) {
    pspAudioRunning = false;
    pspUserAudioCallback = callback;
    pspUserAudioData = userdata;
    
    pspAudioInit();
    
    sceKernelDelayThread(50000);
    
    pspAudioRunning = true;
    pspAudioSetChannelCallback(0, AudioThread, nullptr);
    
    return 1;
}

inline void ReleaseAudio() {
    pspAudioRunning = false;
    pspAudioEnd();
}

inline void PauseAudio() {
    pspAudioRunning = false;
}

inline void ResumeAudio() {
    pspAudioRunning = true;
}

inline bool IsAudioPaused() {
    return !pspAudioRunning;
}

}

#endif
#endif
