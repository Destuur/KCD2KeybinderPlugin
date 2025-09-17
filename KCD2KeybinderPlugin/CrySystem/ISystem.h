#pragma once

struct IGame; // Forward Declaration

struct IScriptSystem
{
	void RegisterFunction(const char* name, void* func) {}
};

struct ISystem
{
	IScriptSystem* GetIScriptSystem() { return &mScriptSystem; }
	IScriptSystem mScriptSystem;
};

struct SSystemGlobalEnvironment
{
	ISystem* pSystem;
	IGame* pGame;
};

// **Nur Deklaration im Header**
extern SSystemGlobalEnvironment* gEnv;
