#include "RetroEngine.hpp"

#if RETRO_USE_MOD_LOADER || !RETRO_USE_ORIGINAL_CODE
char savePath[0x100];
#endif

#if RETRO_USE_MOD_LOADER
std::vector<ModInfo> modList;
int activeMod = -1;

char modsPath[0x100];

bool redirectSave           = false;
bool disableSaveIniOverride = false;

char modTypeNames[OBJECT_COUNT][0x40];
char modScriptPaths[OBJECT_COUNT][0x40];
byte modScriptFlags[OBJECT_COUNT];
byte modObjCount = 0;

char playerNames[PLAYERNAME_COUNT][0x20];
byte playerCount = 0;

#include <algorithm>

#if RETRO_PLATFORM == RETRO_PSP
#include <pspiofilemgr.h>
#include <cstring>
#include <string>

struct PspDirEntry {
    std::string name;
    bool isDirectory;
};

bool pspDirectoryExists(const char* path) {
    SceUID dir = sceIoDopen(path);
    if (dir >= 0) {
        sceIoDclose(dir);
        return true;
    }
    return false;
}

bool pspFileExists(const char* path) {
    SceIoStat stat;
    return sceIoGetstat(path, &stat) >= 0;
}

std::vector<PspDirEntry> pspScanDirectory(const char* path) {
    std::vector<PspDirEntry> entries;
    SceUID dir = sceIoDopen(path);
    if (dir >= 0) {
        SceIoDirent dirent;
        while (sceIoDread(dir, &dirent) > 0) {
            if (strcmp(dirent.d_name, ".") == 0 || strcmp(dirent.d_name, "..") == 0)
                continue;
            PspDirEntry entry;
            entry.name = dirent.d_name;
            entry.isDirectory = FIO_S_ISDIR(dirent.d_stat.st_mode);
            entries.push_back(entry);
        }
        sceIoDclose(dir);
    }
    return entries;
}

void pspScanDirectoryRecursive(const char* basePath, std::vector<std::string>& files) {
    SceUID dir = sceIoDopen(basePath);
    if (dir >= 0) {
        SceIoDirent dirent;
        while (sceIoDread(dir, &dirent) > 0) {
            if (strcmp(dirent.d_name, ".") == 0 || strcmp(dirent.d_name, "..") == 0)
                continue;
            
            char fullPath[0x200];
            sprintf(fullPath, "%s/%s", basePath, dirent.d_name);
            
            if (FIO_S_ISDIR(dirent.d_stat.st_mode)) {
                pspScanDirectoryRecursive(fullPath, files);
            } else {
                files.push_back(std::string(fullPath));
            }
        }
        sceIoDclose(dir);
    }
}

#else
#include <filesystem>

#if RETRO_PLATFORM == RETRO_ANDROID
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif
#endif

int OpenModMenu()
{
    // Engine.gameMode      = ENGINE_INITMODMENU;
    // Engine.modMenuCalled = true;
    return 1;
}

#if RETRO_PLATFORM == RETRO_PSP
void InitMods()
{
    modList.clear();
    forceUseScripts        = forceUseScripts_Config;
    disableFocusPause      = disableFocusPause_Config;
    redirectSave           = false;
    disableSaveIniOverride = false;
    sprintf(savePath, "");

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);

    if (pspDirectoryExists(modBuf)) {
        char configPath[0x100];
        sprintf(configPath, "%s/modconfig.ini", modBuf);
        
        FileIO *configFile = fOpen(configPath, "r");
        if (configFile) {
            fClose(configFile);
            IniParser modConfig(configPath, false);

            for (int m = 0; m < (int)modConfig.items.size(); ++m) {
                bool active = false;
                ModInfo info;
                modConfig.GetBool("mods", modConfig.items[m].key, &active);
                if (LoadMod(&info, std::string(modBuf), modConfig.items[m].key, active))
                    modList.push_back(info);
            }
        }

        std::vector<PspDirEntry> entries = pspScanDirectory(modBuf);
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].isDirectory) {
                std::string folder = entries[i].name;

                bool flag = true;
                for (int m = 0; m < (int)modList.size(); ++m) {
                    if (modList[m].folder == folder) {
                        flag = false;
                        break;
                    }
                }

                if (flag) {
                    ModInfo info;
                    if (LoadMod(&info, std::string(modBuf), folder, false))
                        modList.push_back(info);
                }
            }
        }
    }

    disableFocusPause = disableFocusPause_Config;
    forceUseScripts   = forceUseScripts_Config;
    sprintf(savePath, "");
    redirectSave           = false;
    disableSaveIniOverride = false;
    for (int m = 0; m < (int)modList.size(); ++m) {
        if (!modList[m].active)
            continue;
        if (modList[m].useScripts)
            forceUseScripts = true;
        if (modList[m].disableFocusPause)
            disableFocusPause |= modList[m].disableFocusPause;
        if (modList[m].redirectSave) {
            sprintf(savePath, "%s", modList[m].savePath.c_str());
            redirectSave = true;
        }
        if (modList[m].disableSaveIniOverride)
            disableSaveIniOverride = true;
    }

    ReadSaveRAMData();
    ReadUserdata();
}
#else
fs::path ResolvePath(fs::path given)
{
    if (given.is_relative())
        given = fs::current_path() / given; // thanks for the weird syntax!

    for (auto &p : fs::directory_iterator{ given.parent_path() }) {
        char pbuf[0x100];
        char gbuf[0x100];
        auto pf   = p.path().filename();
        auto pstr = pf.string();
        std::transform(pstr.begin(), pstr.end(), pstr.begin(), [](char c) { return std::tolower(c); });
        auto gf   = given.filename();
        auto gstr = gf.string();
        std::transform(gstr.begin(), gstr.end(), gstr.begin(), [](char c) { return std::tolower(c); });
        if (pstr == gstr) {
            return p.path();
        }
    }
    return given; // might work might not!
}

void InitMods()
{
    modList.clear();
    forceUseScripts        = forceUseScripts_Config;
    disableFocusPause      = disableFocusPause_Config;
    redirectSave           = false;
    disableSaveIniOverride = false;
    sprintf(savePath, "");

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);
    fs::path modPath = ResolvePath(modBuf);

    if (fs::exists(modPath) && fs::is_directory(modPath)) {
        std::string mod_config = modPath.string() + "/modconfig.ini";
        FileIO *configFile     = fOpen(mod_config.c_str(), "r");
        if (configFile) {
            fClose(configFile);
            IniParser modConfig(mod_config.c_str(), false);

            for (int m = 0; m < modConfig.items.size(); ++m) {
                bool active = false;
                ModInfo info;
                modConfig.GetBool("mods", modConfig.items[m].key, &active);
                if (LoadMod(&info, modPath.string(), modConfig.items[m].key, active))
                    modList.push_back(info);
            }
        }

        try {
            auto rdi = fs::directory_iterator(modPath);
            for (auto de : rdi) {
                if (de.is_directory()) {
                    fs::path modDirPath = de.path();

                    ModInfo info;

                    std::string modDir            = modDirPath.string().c_str();
                    const std::string mod_inifile = modDir + "/mod.ini";
                    std::string folder            = modDirPath.filename().string();

                    bool flag = true;
                    for (int m = 0; m < modList.size(); ++m) {
                        if (modList[m].folder == folder) {
                            flag = false;
                            break;
                        }
                    }

                    if (flag) {
                        if (LoadMod(&info, modPath.string(), modDirPath.filename().string(), false))
                            modList.push_back(info);
                    }
                }
            }
        } catch (fs::filesystem_error fe) {
            PrintLog("Mods Folder Scanning Error: ");
            PrintLog(fe.what());
        }
    }

    disableFocusPause = disableFocusPause_Config;
    forceUseScripts   = forceUseScripts_Config;
    sprintf(savePath, "");
    redirectSave           = false;
    disableSaveIniOverride = false;
    for (int m = 0; m < modList.size(); ++m) {
        if (!modList[m].active)
            continue;
        if (modList[m].useScripts)
            forceUseScripts = true;
        if (modList[m].disableFocusPause)
            disableFocusPause |= modList[m].disableFocusPause;
        if (modList[m].redirectSave) {
            sprintf(savePath, "%s", modList[m].savePath.c_str());
            redirectSave = true;
        }
        if (modList[m].disableSaveIniOverride)
            disableSaveIniOverride = true;
    }

    ReadSaveRAMData();
    ReadUserdata();
}
#endif

bool LoadMod(ModInfo *info, std::string modsPath, std::string folder, bool active)
{
    if (!info)
        return false;

    info->fileMap.clear();
    info->name    = "";
    info->desc    = "";
    info->author  = "";
    info->version = "";
    info->folder  = "";
    info->active  = false;

    const std::string modDir = modsPath + "/" + folder;

    FileIO *f = fOpen((modDir + "/mod.ini").c_str(), "r");
    if (f) {
        fClose(f);
        IniParser modSettings((modDir + "/mod.ini").c_str(), false);

        info->name    = "Unnamed Mod";
        info->desc    = "";
        info->author  = "Unknown Author";
        info->version = "1.0.0";
        info->folder  = folder;

        char infoBuf[0x100];
        // Name
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Name", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->name = infoBuf;
        // Desc
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Description", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->desc = infoBuf;
        // Author
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Author", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->author = infoBuf;
        // Version
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Version", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->version = infoBuf;

        info->active = active;

        if (active) {
            ScanModFolder(info);
        }

        info->useScripts = false;
        modSettings.GetBool("", "TxtScripts", &info->useScripts);
        if (info->useScripts && info->active)
            forceUseScripts = true;

        info->disableFocusPause = false;
        modSettings.GetInteger("", "DisableFocusPause", &info->disableFocusPause);
        if (info->disableFocusPause && info->active)
            disableFocusPause |= info->disableFocusPause;

        info->redirectSave = false;
        modSettings.GetBool("", "RedirectSaveRAM", &info->redirectSave);
        if (info->redirectSave) {
            char path[0x100];
            sprintf(path, "mods/%s/", folder.c_str());
            info->savePath = path;
        }

        info->disableSaveIniOverride = false;
        modSettings.GetBool("", "DisableSaveIniOverride", &info->disableSaveIniOverride);
        if (info->disableSaveIniOverride && info->active)
            disableSaveIniOverride = true;

        return true;
    }
    return false;
}

#if RETRO_PLATFORM == RETRO_PSP
void pspAddFilesToModMap(ModInfo *info, const char* basePath, const char* folderName) {
    char searchPath[0x200];
    sprintf(searchPath, "%s/%s", basePath, folderName);
    
    if (!pspDirectoryExists(searchPath))
        return;
    
    std::vector<std::string> files;
    pspScanDirectoryRecursive(searchPath, files);
    
    char folderTest[2][0x20];
    sprintf(folderTest[0], "%s/", folderName);
    sprintf(folderTest[1], "%s\\", folderName);
    
    for (size_t i = 0; i < files.size(); i++) {
        char modBuf[0x200];
        StrCopy(modBuf, files[i].c_str());
        
        int tokenPos = -1;
        for (int t = 0; t < 2; ++t) {
            tokenPos = FindStringToken(modBuf, folderTest[t], 1);
            if (tokenPos >= 0)
                break;
        }

        if (tokenPos >= 0) {
            char buffer[0x200];
            for (int j = StrLength(modBuf); j >= tokenPos; --j) {
                buffer[j - tokenPos] = modBuf[j] == '\\' ? '/' : modBuf[j];
            }

            char pathLower[0x200];
            memset(pathLower, 0, sizeof(pathLower));
            for (int c = 0; buffer[c]; ++c) {
                pathLower[c] = tolower(buffer[c]);
            }

            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
        }
    }
}

void ScanModFolder(ModInfo *info)
{
    if (!info)
        return;

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);
    
    char modDir[0x200];
    sprintf(modDir, "%s/%s", modBuf, info->folder.c_str());

    pspAddFilesToModMap(info, modDir, "Data");
    pspAddFilesToModMap(info, modDir, "Scripts");
    pspAddFilesToModMap(info, modDir, "Videos");    
    char dataDir[0x200];
    sprintf(dataDir, "%s/Data", modDir);
    pspAddFilesToModMap(info, dataDir, "Scripts");
    pspAddFilesToModMap(info, dataDir, "Videos");
}
#else
void ScanModFolder(ModInfo *info)
{
    if (!info)
        return;

    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);

    fs::path modPath = ResolvePath(modBuf);

    const std::string modDir = modPath.string() + "/" + info->folder;

    // Check for Data/ replacements
    fs::path dataPath = ResolvePath(modDir + "/Data");

    if (fs::exists(dataPath) && fs::is_directory(dataPath)) {
        try {
            auto data_rdi = fs::recursive_directory_iterator(dataPath);
            for (auto data_de : data_rdi) {
                if (data_de.is_regular_file()) {
                    char modBuf[0x100];
                    StrCopy(modBuf, data_de.path().string().c_str());
                    char folderTest[4][0x10] = {
                        "Data/",
                        "Data\\",
                        "data/",
                        "data\\",
                    };
                    int tokenPos = -1;
                    for (int i = 0; i < 4; ++i) {
                        tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                        if (tokenPos >= 0)
                            break;
                    }

                    if (tokenPos >= 0) {
                        char buffer[0x100];
                        for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                            buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                        }

                        // PrintLog(modBuf);
                        std::string path(buffer);
                        std::string modPath(modBuf);
                        char pathLower[0x100];
                        memset(pathLower, 0, sizeof(char) * 0x100);
                        for (int c = 0; c < path.size(); ++c) {
                            pathLower[c] = tolower(path.c_str()[c]);
                        }

                        info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                    }
                }
            }
        } catch (fs::filesystem_error fe) {
            PrintLog("Data Folder Scanning Error: ");
            PrintLog(fe.what());
        }
    }

    // Check for Scripts/ replacements
    fs::path scriptPath = ResolvePath(modDir + "/Scripts");

    if (fs::exists(scriptPath) && fs::is_directory(scriptPath)) {
        try {
            auto data_rdi = fs::recursive_directory_iterator(scriptPath);
            for (auto data_de : data_rdi) {
                if (data_de.is_regular_file()) {
                    char modBuf[0x100];
                    StrCopy(modBuf, data_de.path().string().c_str());
                    char folderTest[4][0x10] = {
                        "Scripts/",
                        "Scripts\\",
                        "scripts/",
                        "scripts\\",
                    };
                    int tokenPos = -1;
                    for (int i = 0; i < 4; ++i) {
                        tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                        if (tokenPos >= 0)
                            break;
                    }

                    if (tokenPos >= 0) {
                        char buffer[0x100];
                        for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                            buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                        }

                        // PrintLog(modBuf);
                        std::string path(buffer);
                        std::string modPath(modBuf);
                        char pathLower[0x100];
                        memset(pathLower, 0, sizeof(char) * 0x100);
                        for (int c = 0; c < path.size(); ++c) {
                            pathLower[c] = tolower(path.c_str()[c]);
                        }

                        info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                    }
                }
            }
        } catch (fs::filesystem_error fe) {
            PrintLog("Script Folder Scanning Error: ");
            PrintLog(fe.what());
        }
    }

    // Check for Videos/ replacements
    fs::path videosPath = ResolvePath(modDir + "/Videos");

    if (fs::exists(videosPath) && fs::is_directory(videosPath)) {
        try {
            auto data_rdi = fs::recursive_directory_iterator(videosPath);
            for (auto data_de : data_rdi) {
                if (data_de.is_regular_file()) {
                    char modBuf[0x100];
                    StrCopy(modBuf, data_de.path().string().c_str());
                    char folderTest[4][0x10] = {
                        "Videos/",
                        "Videos\\",
                        "videos/",
                        "videos\\",
                    };
                    int tokenPos = -1;
                    for (int i = 0; i < 4; ++i) {
                        tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                        if (tokenPos >= 0)
                            break;
                    }

                    if (tokenPos >= 0) {
                        char buffer[0x100];
                        for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                            buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                        }

                        // PrintLog(modBuf);
                        std::string path(buffer);
                        std::string modPath(modBuf);
                        char pathLower[0x100];
                        memset(pathLower, 0, sizeof(char) * 0x100);
                        for (int c = 0; c < path.size(); ++c) {
                            pathLower[c] = tolower(path.c_str()[c]);
                        }

                        info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                    }
                }
            }
        } catch (fs::filesystem_error fe) {
            PrintLog("Videos Folder Scanning Error: ");
            PrintLog(fe.what());
        }
    }
}
#endif

#if RETRO_PLATFORM == RETRO_PSP
void SaveMods()
{
    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);

    if (pspDirectoryExists(modBuf)) {
        char configPath[0x200];
        sprintf(configPath, "%s/modconfig.ini", modBuf);
        IniParser modConfig;

        for (int m = 0; m < (int)modList.size(); ++m) {
            ModInfo *info = &modList[m];
            modConfig.SetBool("mods", info->folder.c_str(), info->active);
        }

        modConfig.Write(configPath, false);
    }
}
#else
void SaveMods()
{
    char modBuf[0x100];
    sprintf(modBuf, "%smods", modsPath);
    fs::path modPath = ResolvePath(modBuf);

    if (fs::exists(modPath) && fs::is_directory(modPath)) {
        std::string mod_config = modPath.string() + "/modconfig.ini";
        IniParser modConfig;

        for (int m = 0; m < modList.size(); ++m) {
            ModInfo *info = &modList[m];

            modConfig.SetBool("mods", info->folder.c_str(), info->active);
        }

        modConfig.Write(mod_config.c_str(), false);
    }
}
#endif

void RefreshEngine()
{
    // Reload entire engine
    Engine.LoadGameConfig("Data/Game/GameConfig.bin");
#if RETRO_USING_SDL2
    if (Engine.window) {
        char gameTitle[0x40];
        sprintf(gameTitle, "%s%s", Engine.gameWindowText, Engine.usingDataFile_Config ? "" : " (Using Data Folder)");
        SDL_SetWindowTitle(Engine.window, gameTitle);
    }
#elif RETRO_USING_SDL1
    char gameTitle[0x40];
    sprintf(gameTitle, "%s%s", Engine.gameWindowText, Engine.usingDataFile_Config ? "" : " (Using Data Folder)");
    SDL_WM_SetCaption(gameTitle, NULL);
#endif

    ReleaseGlobalSfx();
    LoadGlobalSfx();

    disableFocusPause = disableFocusPause_Config;
    forceUseScripts   = forceUseScripts_Config;
    sprintf(savePath, "");
    redirectSave           = false;
    disableSaveIniOverride = false;
    for (int m = 0; m < modList.size(); ++m) {
        if (!modList[m].active)
            continue;
        if (modList[m].useScripts)
            forceUseScripts = true;
        if (modList[m].disableFocusPause)
            disableFocusPause |= modList[m].disableFocusPause;
        if (modList[m].redirectSave) {
            sprintf(savePath, "%s", modList[m].savePath.c_str());
            redirectSave = true;
        }
        if (modList[m].disableSaveIniOverride)
            disableSaveIniOverride = true;
    }

    SaveMods();

    ReadSaveRAMData();
    ReadUserdata();
}

#endif

#if RETRO_USE_MOD_LOADER || !RETRO_USE_ORIGINAL_CODE
int GetSceneID(byte listID, const char *sceneName)
{
    if (listID >= 3)
        return -1;

    char scnName[0x40];
    int scnPos = 0;
    int pos    = 0;
    while (sceneName[scnPos]) {
        if (sceneName[scnPos] != ' ')
            scnName[pos++] = sceneName[scnPos];
        ++scnPos;
    }
    scnName[pos] = 0;

    for (int s = 0; s < stageListCount[listID]; ++s) {
        char nameBuffer[0x40];

        scnPos = 0;
        pos    = 0;
        while (stageList[listID][s].name[scnPos]) {
            if (stageList[listID][s].name[scnPos] != ' ')
                nameBuffer[pos++] = stageList[listID][s].name[scnPos];
            ++scnPos;
        }
        nameBuffer[pos] = 0;

        if (StrComp(scnName, nameBuffer)) {
            return s;
        }
    }
    return -1;
}
#endif
