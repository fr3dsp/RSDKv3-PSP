#include "RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_PSP
#include <pspkernel.h>
#include <psppower.h>

PSP_MODULE_INFO("SonicCD", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

volatile bool pspSuspended = false;

ushort* pspVramBuffer = nullptr;
u64 pspTickFrequency = 0;
volatile bool pspAudioRunning = false;
void (*pspUserAudioCallback)(void*, unsigned int, void*) = nullptr;
void* pspUserAudioData = nullptr;
unsigned int __attribute__((aligned(16))) pspDisplayList[256];

int pspExitCallback(int arg1, int arg2, void *common) {
    Engine.running = false;
    return 0;
}

int pspPowerCallback(int unknown, int pwrflags, void *common) {
    if (pwrflags & PSP_POWER_CB_POWER_SWITCH || pwrflags & PSP_POWER_CB_SUSPENDING) {
        pspSuspended = true;
    } else if (pwrflags & PSP_POWER_CB_RESUMING || pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
        pspSuspended = false;
    }
    return 0;
}

int pspCallbackThread(SceSize args, void *argp) {
    int cbid;
    
    cbid = sceKernelCreateCallback("Exit Callback", pspExitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    
    cbid = sceKernelCreateCallback("Power Callback", pspPowerCallback, NULL);
    scePowerRegisterCallback(0, cbid);
    
    sceKernelSleepThreadCB();
    return 0;
}

void pspSetupCallbacks() {
    int thid = sceKernelCreateThread("psp_callback_thread", pspCallbackThread, 0x11, 0xFA0, 0, NULL);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, NULL);
    }
}
#endif

#if !RETRO_USE_ORIGINAL_CODE

#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
#include "Windows.h"
#endif

void parseArguments(int argc, char *argv[])
{
    for (int a = 0; a < argc; ++a) {
        const char *find = "";

        find = strstr(argv[a], "stage=");
        if (find) {
            int b = 0;
            int c = 6;
            while (find[c] && find[c] != ';') Engine.startSceneFolder[b++] = find[c++];
            Engine.startSceneFolder[b] = 0;
        }

        find = strstr(argv[a], "scene=");
        if (find) {
            int b = 0;
            int c = 6;
            while (find[c] && find[c] != ';') Engine.startSceneID[b++] = find[c++];
            Engine.startSceneID[b] = 0;
        }

        find = strstr(argv[a], "console=true");
        if (find) {
            engineDebugMode       = true;
            Engine.devMenu        = true;
            Engine.consoleEnabled = true;
#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
            AllocConsole();
            freopen_s((FILE **)stdin, "CONIN$", "w", stdin);
            freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);
#endif
        }

        find = strstr(argv[a], "usingCWD=true");
        if (find) {
            usingCWD = true;
        }
    }
}
#endif

int main(int argc, char *argv[])
{
#if RETRO_PLATFORM == RETRO_PSP
    scePowerSetClockFrequency(333, 333, 166);
    pspSetupCallbacks();
#endif

#if !RETRO_USE_ORIGINAL_CODE
    parseArguments(argc, argv);
#endif

    Engine.Init();
    Engine.Run();

#if !RETRO_USE_ORIGINAL_CODE
    if (Engine.consoleEnabled) {
#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
        FreeConsole();
#endif
    }
#endif

#if RETRO_PLATFORM == RETRO_PSP
    sceKernelExitGame();
#endif

    return 0;
}
