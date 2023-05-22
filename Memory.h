#pragma once

#include <Windows.h>

extern volatile DWORD hsh;

bool TestSpecByte();
bool TestMemoryBreak();
bool TestHardBreak();
bool TestFuncPatch();
bool TestHashsum(int);