/**
* some of the techniques realisations was taken from LordNoteworthy/al-khaser, CheckPointSW/showstopper and https://anti-debug.checkpoint.com/
* this program is a combination of the most antidebug techniques with some new ideas in way of realisation of them, it's combination and time checks. 
*
*/


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
#include <cwctype>
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <fstream>
#include <atomic>
#include "defs.h"
#include "Flags.h"
#include "ObjectHandles.h"
#include "Exceptions.h"
#include "Memory.h"
#include "Assembly.h"
#include "Misc.h"
#include "Timing.h"
#include "Config.h"
#include <string.h>


bool withtls = false;

void WINAPI TlsCallback(PVOID pMod, DWORD Reas, PVOID Con)
{
    //just some tlscallbacs for hiding;
}

#pragma comment (linker, "/INCLUDE:__tls_used")
#pragma comment (linker, "/INCLUDE:_tls_callback_func")
#pragma comment (linker, "/INCLUDE:_tls_callback_func2")

#ifdef _WIN64
#pragma const_seg(".CRT$XLF")
EXTERN_C const
#else
#pragma data_seg(".CRT$XLF")
EXTERN_C
#endif
PIMAGE_TLS_CALLBACK tls_callback_func = TlsCallback;
#ifdef _WIN64
#pragma const_seg()
#else
#pragma data_seg()
#endif //_WIN64



void WINAPI TlsCallback2(PVOID pMod, DWORD Reas, PVOID Con)
{
    if (withtls) {
        std::cout << "Tls TestCreateToolhelp32Snapshot:" << TestCreateToolhelp32Snapshot() << std::endl;
        std::cout << "Tls SuspendDebuggerThread:" << SuspendDebuggerThread() << std::endl;
        std::cout << "Tls TestIsDebuggerPresent:" << TestIsDebuggerPresent() << std::endl;
        std::cout << "Tls TestCheckRemoteDebuggerPresent:" << TestCheckRemoteDebuggerPresent() << std::endl;
        std::cout << "Tls TestProcessDebugPort:" << TestProcessDebugPort() << std::endl;
        std::cout << "Tls TestProcessDebugFlags:" << TestProcessDebugFlags() << std::endl;
        std::cout << "Tls TestSystemKernelDebuggerInformation:" << TestSystemKernelDebuggerInformation() << std::endl;
        std::cout << "Tls TestNtGlobalFlag:" << TestNtGlobalFlag() << std::endl;
        std::cout << "Tls TestHeapCheck:" << TestHeapCheck() << std::endl;
        std::cout << "Tls TestFindWindow:" << TestFindWindow() << std::endl;
        std::cout << "Tls TestCreateFile:" << TestCreateFile() << std::endl;
        std::cout << "Tls TestCloseHandle:" << TestCloseHandle() << std::endl;
        std::cout << "Tls TestLoadLibrary:" << TestLoadLibrary() << std::endl;
        std::cout << "Tls TestCsrGetProcessId:" << TestCsrGetProcessId() << std::endl;
        std::cout << "Tls TestQueryObject:" << TestQueryObject() << std::endl;
        std::cout << "Tls Patch_DbgUiRemoteBreakin:" << Patch_DbgUiRemoteBreakin() << std::endl;
        std::cout << "Tls TestHideThread:" << HideThread();
        std::cout << std::endl;
    }
}

#ifdef _WIN64
#pragma const_seg(".CRT$XLG")
EXTERN_C const
#else
#pragma data_seg(".CRT$XLG")
EXTERN_C
#endif
PIMAGE_TLS_CALLBACK tls_callback_func2 = TlsCallback2;
#ifdef _WIN64
#pragma const_seg()
#else
#pragma data_seg()
#endif //_WIN64

std::string btos(bool a) {
    if (a) return "1";
    return "0";
}

#pragma auto_inline(off)
int main(int argc, char** argv) {
    std::string confpath = "config.txt";
    if (argc == 2) { confpath = std::string(argv[1]);  }
    config::IniConf conf(confpath);
    config::readconfig(conf);
    std::vector<std::thread> threads;
    for (size_t i = 0; i < config::threads; ++i) {
        threads.push_back(std::thread([]() { while (1) Sleep(1000); }));
    }
    for (auto ftion : config::launch) {
        if (ftion.second == "yes") {
            if (ftion.first == "TestSelfdebug") {
                threads.push_back(std::thread([=]() { while (1) {
                    bool res;
                    tstdebug f = (tstdebug)config::functions["TestSelfdebug"];
                    auto wtime = timer_timeGetTime([&]() { res = f(argc, argv); });
                    if (wtime > 200) res = true; std::cout << ftion.first + ": " + btos(res) + "\n"; Sleep(1000);
                }}));
            }
            else if (ftion.first == "TestHashsum") {
                threads.push_back(std::thread([=]() { while (1) {
                    bool res;
                    tsthshsum f = (tsthshsum)config::functions["TestHashsum"];
                    auto wtime = timer_timeGetTime([&]() { res = f(config::hash); });
                    if (wtime > 200) res = true; std::cout << ftion.first + ": " + btos(res) + "\n"; Sleep(1000);
                }}));
            }
            else if (ftion.first.find("time") != std::string::npos && ftion.first != "timer_partstiming") {
                threads.push_back(std::thread([=]() { while (1) {
                    bool res;
                    auto f = (timerfunc)config::functions[ftion.first];
                    res = timer_usualtiming(f); std::cout << ftion.first + ": " + btos(res) + "\n"; Sleep(1000);
                }}));
            }
            else {
                threads.push_back(std::thread([=]() { while (1) {
                    bool res;
                    tstdefault f = (tstdefault)config::functions[ftion.first];
                    auto wtime = timer_timeGetTime([&]() { res = f(); });
                    if (wtime > 200 && ftion.first != "timer_partstiming") res = true;
                    std::cout << ftion.first + ": " + btos(res) + "\n"; Sleep(1000);
                }}));
            }

        }
    }
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].detach();
    }
    getchar();
}