#include "dllmain.h"

HMODULE dllHandle;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			dllHandle = hModule;
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		default: ;
	}
	return TRUE;
}
