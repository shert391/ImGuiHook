#pragma once
#include <list>

struct SaveData {
	void* Pointer;
	size_t Lenght;
	byte* Bytes;
};

class Hook
{
private:
	static std::list<SaveData> _savesData;
public:
	static void HookFunction(void* dst, void** from, int nopSize);
	static void UnhookAll();
};

