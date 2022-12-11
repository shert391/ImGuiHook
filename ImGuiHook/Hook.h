#pragma once
class Hook
{
public:
	static void HookFunction(void* dst, void** from, int nopSize);
};

