#include "pch.h"
#include <windows.h>
#include <thread>

// Vorwärtsdeklaration
DWORD WINAPI MainThread(LPVOID);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		// Cleanup Hooks hier (optional)
		break;
	}
	return TRUE;
}
