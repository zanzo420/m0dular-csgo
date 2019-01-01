#ifndef WINDOWS_LOADER_H
#define WINDOWS_LOADER_H

#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

//TODO: Don't rely on LTO removing all WinLoader code not meant to be accessible to the clients

//Windows CSGO is 32 bit and we are currently targetting 32 bit systems
using nptr_t = uint32_t;

#ifndef _MSC_VER
#define __stdcall
#endif

using GetProcAddressFn = int(__stdcall*)(void*, char*);
using LoadLibraryAFn = void*(__stdcall*)(char*);

struct WinSection
{
    uint32_t bufOffset;
	uint32_t virtOffset;
	uint32_t bufSize;
	uint32_t virtSize;
	uint32_t prot;
};

struct WinRelocation
{
	uint32_t bufOffset;
};

//Name offset will be inverted if the value has to be imported by ordinal instead of name
struct WinImport
{
	uint32_t nameOffset;
    uint32_t virtOffset;
};

struct WinImportH
{
	void* module;
	WinImport imp;
};

struct WinImportThunk
{
    uint32_t moduleNameOffset;
    uint32_t importCount;
    uint32_t importOffset;
};

struct WinModule;

struct PackedWinModule
{
	uint32_t modBufSize;
	char* moduleBuffer;
	uint32_t bufSize;
	uint32_t allocSize;
	char* buffer;
	uint32_t entryPointOffset;
    uint32_t sectionOffset;
	uint32_t nameOffset;
    uint32_t importsWHOffset;
    uint32_t importsOffset;
    uint32_t importThunksOffset;
    uint32_t relocationOffset;

	PackedWinModule(const WinModule& mod);

	PackedWinModule(const PackedWinModule& o)
	{
		memcpy(this, &o, sizeof(*this));
		moduleBuffer = nullptr;
		buffer = nullptr;
	}

	PackedWinModule& operator=(const PackedWinModule& o)
	{
		memcpy(this, &o, sizeof(*this));
		moduleBuffer = nullptr;
		buffer = nullptr;
		return *this;
	}

	~PackedWinModule()
	{
		if (buffer)
			free(buffer);
		if (moduleBuffer)
			free(moduleBuffer);
	}

	void PerformRelocations(nptr_t base);

	//This is only meant to be available to the client side
#ifdef _WIN32
	void SetupInPlace(HANDLE processHandle, char* targetModuleAddress, char* targetDataAddress);
#endif
};

struct WinModule
{
	char* moduleBuffer;

	std::vector<WinSection> sections;

	uint32_t entryPointOffset;
	uint32_t moduleSize;

	std::vector<char> names;
	std::vector<WinImportH> importsWH;
	std::vector<WinImport> thunkedImports;
	std::vector<WinImportThunk> importThunk;
	std::vector<WinRelocation> relocations;

	WinModule()
		: moduleBuffer()
	{
	}

	WinModule(const char* buf, size_t size, bool is64 = false);

	uint32_t VirtToFile(uint32_t virtOffset, WinSection*& hint);
	uint32_t FileToVirt(uint32_t virtOffset, WinSection*& hint);

	~WinModule()
	{
		if (moduleBuffer)
			free(moduleBuffer);
	}

};

struct WinLoadData
{
	PackedWinModule* packedModule;
	char* outBuf;
	GetProcAddressFn pGetProcAddress;
	LoadLibraryAFn pLoadLibraryA;
};

#ifdef _WIN32
unsigned long __stdcall LoadPackedModule(void* loadData);
#endif

#endif
