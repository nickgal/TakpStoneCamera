#include <cstdint>
#include <windows.h>
#include <tlhelp32.h>
#include "MinHook.h"

// --- Constants & Offsets ---
const uintptr_t EQ_UI_STATE = 0x0063B918;
const uint8_t EQ_UI_STATE_CLASSIC = 1;

const uintptr_t EQ_POINTER_CEverQuest = 0x00809478;
const uintptr_t EQ_OFFSET_CEverQuest_GAMESTATE = 0x05AC;
const uint32_t EQ_GAME_STATE_IN_GAME = 5;

const uintptr_t EQ_POINTER_EQ_Main = 0x007f9574;
const uintptr_t EQ_OFFSET_EQ_Main_IS_INVENTORY_OPEN = 0x98b78;

const uintptr_t g_bNewUI = 0x008092d8;
const uintptr_t EQ_OFFSET_EQGfx_Dx8_g_bForceClear = 0x637874;

const uintptr_t LoadGfxDLL_Addr = 0x0054999c;

// --- Helper Functions ---
bool EQ_IsInGame() {
    uintptr_t cEverQuest = *(uintptr_t*)EQ_POINTER_CEverQuest;
    if (!cEverQuest) return false;
    return *(uint32_t*)(cEverQuest + EQ_OFFSET_CEverQuest_GAMESTATE) == EQ_GAME_STATE_IN_GAME;
}

bool EQ_IsClassicUI() {
    return *(uint8_t*)g_bNewUI == 0 && *(uint8_t*)EQ_UI_STATE == EQ_UI_STATE_CLASSIC;
}

bool EQ_IsInventoryOpen() {
    uintptr_t eqMain = *(uintptr_t*)EQ_POINTER_EQ_Main;
    if (!eqMain) return false;
    return *(uint32_t*)(eqMain + EQ_OFFSET_EQ_Main_IS_INVENTORY_OPEN) == 1;
}

// --- Hooking t3dSetCameraRenderWindow ---
typedef void(__cdecl* t3dSetCameraRenderWindow_t)(void* arg1, void* arg2, void* arg3);
t3dSetCameraRenderWindow_t fpT3dSetCameraRenderWindow = nullptr;

void __cdecl Hooked_t3dSetCameraRenderWindow(void* arg1, void* arg2, void* arg3) {
    // Call original function
    fpT3dSetCameraRenderWindow(arg1, arg2, arg3);

    HMODULE hGfxDll = GetModuleHandleA("eqgfx_dx8.dll");
    if (hGfxDll) {
        int* g_bForceClear = (int*)((uintptr_t)hGfxDll + EQ_OFFSET_EQGfx_Dx8_g_bForceClear);

        if (EQ_IsInGame() && EQ_IsClassicUI() && EQ_IsInventoryOpen() && *g_bForceClear == 1) {
            *g_bForceClear = 0;
        }
    }
}

void FixInvCamera() {
    HMODULE hGfxDll = GetModuleHandleA("eqgfx_dx8.dll");
    if (!hGfxDll) return;

    void* t3dSetCameraRenderWindow = (void*)GetProcAddress(hGfxDll, "t3dSetCameraRenderWindow");
    if (!t3dSetCameraRenderWindow) return;

    // Create MinHook for the exported function
    if (MH_CreateHook(t3dSetCameraRenderWindow, &Hooked_t3dSetCameraRenderWindow, reinterpret_cast<LPVOID*>(&fpT3dSetCameraRenderWindow)) == MH_OK) {
        MH_EnableHook(t3dSetCameraRenderWindow);
    }
}

// --- Hooking LoadGfxDLL ---
typedef void(__cdecl* LoadGfxDLL_t)();
LoadGfxDLL_t fpLoadGfxDLL = nullptr;

void __cdecl Hooked_LoadGfxDLL() {
    fpLoadGfxDLL(); // Call original execution
    FixInvCamera();
}

// --- Initialization Thread ---
DWORD WINAPI MainThread(LPVOID lpParam) {
    if (MH_Initialize() != MH_OK) return 1;

    if (GetModuleHandleA("eqgfx_dx8.dll") != nullptr) {
        // already loaded, hook immediately
        FixInvCamera();
    } else {
        // Not loaded yet, hook the loader function
        if (MH_CreateHook((LPVOID)LoadGfxDLL_Addr, &Hooked_LoadGfxDLL, reinterpret_cast<LPVOID*>(&fpLoadGfxDLL)) == MH_OK) {
            MH_EnableHook((LPVOID)LoadGfxDLL_Addr);
        }
    }
    return 0;
}

// --- DLL Entry Point ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;
}
