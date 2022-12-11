#include <Windows.h>
#include "Hook.h"

std::list<SaveData> Hook::_savesData;

void Hook::HookFunction(void* dst, void** from, int nopSize)
{
	DWORD oldProtect = 0;
	void* pit = VirtualAlloc(nullptr, nopSize + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	VirtualProtect(*from, nopSize, PAGE_EXECUTE_READWRITE, &oldProtect);
	SaveData saveDate{ *from, nopSize };
	saveDate.Bytes = new byte[nopSize];
	memcpy(saveDate.Bytes, *from, nopSize);
	_savesData.push_back(saveDate);
	if (*(byte*)*from == 0xE9) {
		*(char*)pit = 0xE9;
		*(DWORD*)((DWORD)pit + 1) = (5 + *(DWORD*)((DWORD)*from + 1) + (DWORD)(*from)) - (DWORD)pit - 5;
	}
	else
	{
		memcpy(pit, *from, nopSize);
	}
	memset(*from, 0x90, nopSize);
	DWORD opcodeJmp = (DWORD)dst - (DWORD)*from - 5;
	**(char**)from = 0xE9;
	*(DWORD*)((DWORD)*from + 1) = opcodeJmp;
	VirtualProtect(*from, nopSize, oldProtect, &oldProtect);
	*(char*)((DWORD)pit + nopSize) = 0xE9;
	opcodeJmp = ((DWORD)*from + nopSize) - ((DWORD)pit + nopSize) - 5;
	*(DWORD*)((DWORD)pit + nopSize + 1) = opcodeJmp;
	*from = pit;
}

void Hook::UnhookAll()
{
	DWORD oldProtect;
	for (SaveData& saveData : _savesData) {
		VirtualProtect(saveData.Pointer, saveData.Lenght, PAGE_EXECUTE_READWRITE, &oldProtect);
		memcpy(saveData.Pointer, saveData.Bytes, saveData.Lenght);
		VirtualProtect(saveData.Pointer, saveData.Lenght, oldProtect, &oldProtect);
	}
}
