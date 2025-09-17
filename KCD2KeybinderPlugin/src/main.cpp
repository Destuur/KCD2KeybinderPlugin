#include "pch.h"
#include <windows.h>
#include <thread>
#include <optional>
#include <libmem/libmem.h>
#include "CrySystem/ISystem.h"
#include "CryGame/IGame.h"
#include "CryScriptSystem/IScriptSystem.h"

// ----------------- Globals -----------------
SSystemGlobalEnvironment* gEnv = nullptr;

// ----------------- Pattern Scan für gEnv -----------------
std::optional<uintptr_t> FindEnvAddr()
{
	lm_module_t module;
	constexpr auto CLIENT_DLL = "WHGame.DLL";

	while (!LM_FindModule(CLIENT_DLL, &module))
		std::this_thread::yield();

	const auto pattern =
		"48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 45 33 C9 45 33 C0 4C 8B 11";

	const auto scan_address = LM_SigScan(pattern, module.base, module.size);
	if (!scan_address)
		return std::nullopt;

	int32_t rip_offset;
	if (!LM_ReadMemory(scan_address + 3,
		reinterpret_cast<lm_byte_t*>(&rip_offset),
		sizeof(rip_offset)))
		return std::nullopt;

	const auto console_addr = scan_address + 3 + 4 + rip_offset;
	return console_addr - 0xA8;
}

// ----------------- Hook: IGame::CompleteInit -----------------
using CompleteInitFunc = bool(__thiscall*)(IGame*);
CompleteInitFunc oCompleteInit = nullptr;

bool __thiscall hkCompleteInit(IGame* pThis)
{
	CryLogAlways("[KCD2NativeHook] Hooked CompleteInit!");

	// Beispiel: Lua-Binding registrieren
	if (gEnv && gEnv->pSystem)
	{
		IScriptSystem* pSS = gEnv->pSystem->GetIScriptSystem();
		if (pSS)
		{
			pSS->RegisterFunction("PrintHello", [](IFunctionHandler* pH) {
				CryLogAlways("[KCD2NativeHook] Lua_PrintHello called!");
				return pH->EndFunction();
				});
		}
	}

	return oCompleteInit(pThis);
}

// ----------------- Hook Setup -----------------
void SetupHooks()
{
	auto envAddr = FindEnvAddr();
	if (!envAddr)
	{
		CryLogAlways("[KCD2NativeHook] Failed to locate gEnv");
		return;
	}

	gEnv = reinterpret_cast<SSystemGlobalEnvironment*>(*envAddr);

	// Warten, bis pGame existiert
	while (!gEnv->pGame)
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	IGame* pGame = gEnv->pGame;
	void** vTable = *reinterpret_cast<void***>(pGame);

	DWORD oldProtect;
	VirtualProtect(&vTable[4], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	oCompleteInit = reinterpret_cast<CompleteInitFunc>(vTable[4]);
	vTable[4] = reinterpret_cast<void*>(&hkCompleteInit);
	VirtualProtect(&vTable[4], sizeof(void*), oldProtect, nullptr);

	CryLogAlways("[KCD2NativeHook] IGame::CompleteInit hooked");
}

// ----------------- MainThread -----------------
DWORD WINAPI MainThread(LPVOID)
{
	CryLogAlways("[KCD2NativeHook] Main thread started");
	SetupHooks();
	return 0;
}
