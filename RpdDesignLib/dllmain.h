#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern HMODULE dllHandle;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
