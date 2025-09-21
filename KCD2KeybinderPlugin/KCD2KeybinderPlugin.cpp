#include "pch.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>

static void Log(const std::string& msg)
{
	std::ofstream f("KCD2Keybinder.log", std::ios::app);
	f << msg << "\n";
	OutputDebugStringA((msg + "\n").c_str());
}

static void WaitForProcessWithMsgPump(HANDLE hProcess)
{
	bool running = true;
	while (running)
	{
		DWORD waitResult = MsgWaitForMultipleObjects(
			1,
			&hProcess,
			FALSE,
			INFINITE,
			QS_ALLINPUT
		);

		if (waitResult == WAIT_OBJECT_0)
		{
			running = false;
		}
		else if (waitResult == WAIT_OBJECT_0 + 1)
		{
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			running = false;
		}
	}
}

static bool HasCommandLineArg(const std::string& arg)
{
	// Ganze Commandline abrufen
	std::string cmd = GetCommandLineA();

	// Alles nach "//" abschneiden (Kommentar-Simulation)
	size_t commentPos = cmd.find("//");
	if (commentPos != std::string::npos)
		cmd = cmd.substr(0, commentPos);

	// Nach Argument suchen
	return cmd.find(arg) != std::string::npos;
}

bool __fastcall Hooked_CompleteInit(void* pGame)
{
	Log("Hooked_CompleteInit triggered!");

	// Prüfen, ob "-keybinder" als Argument vorhanden ist
	if (!HasCommandLineArg("-keybinder"))
	{
		Log("-keybinder flag not present, skipping Keybinder exe launch");
		return true; // einfach weitermachen, ohne zu starten
	}

	char path[MAX_PATH]{ 0 };
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string exeDir(path);
	auto pos = exeDir.find_last_of("\\/");
	exeDir = (pos == std::string::npos) ? exeDir : exeDir.substr(0, pos);

	std::string exePath = exeDir + "\\KCD2Keybinder.exe";

	Log("Starting KCD2Keybinder.exe...");
	STARTUPINFOA si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};

	if (CreateProcessA(exePath.c_str(), nullptr, nullptr, nullptr, FALSE, 0,
		nullptr, nullptr, &si, &pi))
	{
		Log("EXE started, waiting with message pump...");
		WaitForProcessWithMsgPump(pi.hProcess);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		Log("KCD2Keybinder.exe finished, continuing game startup...");
	}
	else
	{
		DWORD err = GetLastError();
		Log("Failed to start KCD2Keybinder.exe! Error=" + std::to_string(err));
	}

	return true;
}

// Minimal DllMain
BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hMod);
		Hooked_CompleteInit(nullptr);
	}
	return TRUE;
}
