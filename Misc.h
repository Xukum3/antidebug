#pragma once

#include <iostream>
#include <unordered_map>
#include <windows.h>
#include <thread>
#include <chrono>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <IPTypes.h>
#include <Shlwapi.h>
#include <Iphlpapi.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <Psapi.h>
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <fstream>
#include <atomic>

bool TestSelfdebug(int argc, char** argv);
bool TestCreateToolhelp32Snapshot();
bool Patch_DbgUiRemoteBreakin();
bool Patch_DbgBreakPoint();
bool TestSwitch();
bool HideThread();
bool SuspendDebuggerThread();
bool TestBlockInput();