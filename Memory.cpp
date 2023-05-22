#include "Memory.h"
#include "defs.h"
#include <TlHelp32.h>
#include <iostream>
#include <thread>

volatile DWORD hsh = 0;

bool Check(BYTE cByte, PVOID pMemory, SIZE_T nMemorySize = 0)
{
    PBYTE pBytes = (PBYTE)pMemory;
    for (SIZE_T i = 0; ; i++)
    {
        if (((nMemorySize > 0) && (i >= nMemorySize)) ||
            ((nMemorySize == 0) && (pBytes[i] == 0xC3)))
            break;

        if (pBytes[i] == cByte)
            return true;
    }
    return false;
}

void MyTestFoo() {
    int x = 1;
}

bool TestSpecByte()
{
    PVOID functionsToCheck[] = {
        &MyTestFoo,
    };
    for (auto funcAddr : functionsToCheck)
    {
        if (Check(0xCC, funcAddr))
            return true;
    }
    return false;
}

bool TestMemoryBreak() {
        DWORD old = 0;
        SYSTEM_INFO sinfo = { 0 };

        GetSystemInfo(&sinfo);
        PVOID pPage = VirtualAlloc(NULL, sinfo.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (NULL == pPage)
            return false;

        PBYTE pMem = (PBYTE)pPage;
        *pMem = 0xC3;

        // Make the page a guard page         
        if (!VirtualProtect(pPage, sinfo.dwPageSize, PAGE_EXECUTE_READWRITE | PAGE_GUARD, &old))
            return false;

        __try
        {
            __asm
            {
                mov eax, pPage
                push mem_bp_being_debugged
                jmp eax
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            VirtualFree(pPage, NULL, MEM_RELEASE);
            return false;
        }

    mem_bp_being_debugged:
        VirtualFree(pPage, NULL, MEM_RELEASE);
        return true;
}

bool TestHardBreak() {
    CONTEXT ctx;
    ZeroMemory(&ctx, sizeof(CONTEXT));
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (!GetThreadContext(GetCurrentThread(), &ctx))
        return false;

    return ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3;
}

bool TestFuncPatch() {
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32)
        return false;

    FARPROC pIsDebuggerPresent = GetProcAddress(hKernel32, "IsDebuggerPresent");
    if (!pIsDebuggerPresent)
        return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot)
        return false;

    PROCESSENTRY32W ProcessEntry;
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &ProcessEntry))
        return false;

    bool bDebuggerPresent = false;
    HANDLE hProcess = NULL;
    DWORD dwFuncBytes = 0;
    const DWORD dwCurrentPID = GetCurrentProcessId();
    do
    {
        __try
        {
            if (dwCurrentPID == ProcessEntry.th32ProcessID)
                continue;

            hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessEntry.th32ProcessID);
            if (NULL == hProcess)
                continue;

            if (!ReadProcessMemory(hProcess, pIsDebuggerPresent, &dwFuncBytes, sizeof(DWORD), NULL))
                continue;

            if (dwFuncBytes != *(PDWORD)pIsDebuggerPresent)
            {
                bDebuggerPresent = true;
                break;
            }
        }
        __finally
        {
            if (hProcess)
                CloseHandle(hProcess);
        }
    } while (Process32NextW(hSnapshot, &ProcessEntry));

    if (hSnapshot)
        CloseHandle(hSnapshot);
    return bDebuggerPresent;
}

#pragma auto_inline(off)
VOID hashfoo()
{
    int x = 0;
    x += 1;
    int y = x + 2;
    x = y - 1;
    return;
};

#pragma auto_inline(off)
DWORD Hash(PBYTE first, DWORD size) {
    DWORD res = 0;
    for (unsigned int i = 0; i < size; ++i) {
        res = (*(first + i) + res) % 1000000;
    }
    return res;
}

#pragma auto_inline(off)
int FindHash(PVOID funcaddr, DWORD funcsize, DWORD funchash, volatile bool& answer)
{
    long res = Hash((PBYTE)funcaddr, funcsize);
    hsh = res;
    if (res != funchash) {
        answer = TRUE;
        return 0;
    }
    return 0;
}

#pragma auto_inline(off)
size_t DetectFunctionSize(PVOID pFunc)
{
    PBYTE pMem = (PBYTE)pFunc;
    size_t fsize = 0;
    do
    {   
        if (*(pMem) == 0xC2 && fsize != 0) break;
        ++fsize;
    } while (*(pMem++) != 0xC3);
    return fsize;
}

#pragma auto_inline(off)
bool TestHashsum(int hash)
{
    volatile bool answer = false;
    PVOID faddr = (VOID*)&hashfoo;
    DWORD fsize = DetectFunctionSize(faddr);
    FindHash(faddr, fsize, (DWORD)hash, answer);
    return answer;
}