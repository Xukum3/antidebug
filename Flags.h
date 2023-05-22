#pragma once
#include <Windows.h>

bool TestIsDebuggerPresent();
bool TestCheckRemoteDebuggerPresent();
bool TestProcessDebugPort();
bool TestProcessDebugFlags();
bool TestSystemKernelDebuggerInformation();
bool TestRtlQuery();
bool TestNtGlobalFlag();
bool TestHeapCheck();