#include "Misc.h"
#include "defs.h"
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
#include <algorithm>
#include <Wbemidl.h>

#define EVENT_SELFDBG_EVENT_NAME L"SelfDebugging"

bool SelfDebugCheck()
{
    WCHAR wszFilePath[MAX_PATH], wszCmdLine[MAX_PATH];
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    HANDLE hDbgEvent;

    hDbgEvent = CreateEventW(NULL, FALSE, FALSE, EVENT_SELFDBG_EVENT_NAME);
    if (!hDbgEvent)
        return false;

    if (!GetModuleFileNameW(NULL, wszFilePath, _countof(wszFilePath)))
        return false;

    swprintf_s(wszCmdLine, L"%s %d", wszFilePath, GetCurrentProcessId());
    if (CreateProcessW(NULL, wszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return WAIT_OBJECT_0 == WaitForSingleObject(hDbgEvent, 0);
    }

    return false;
}

bool EnableDebugPrivilege()
{
    bool bResult = false;
    HANDLE hToken = NULL;
    DWORD ec = 0;

    do
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
            break;

        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
            break;

        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
            break;

        bResult = true;
    } while (0);

    if (hToken)
        CloseHandle(hToken);

    return bResult;
}

bool TestSelfdebug(int argc, char** argv)
{
    if (argc < 2)
    {
        if (SelfDebugCheck())
            return true;
    }
    else
    {
        DWORD dwParentPid = GetCurrentProcessId();
        HANDLE hEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_SELFDBG_EVENT_NAME);
        if (hEvent && EnableDebugPrivilege())
        {
            if (FALSE == DebugActiveProcess(dwParentPid)) {
                return true;
                SetEvent(hEvent);
            }
            else {
                DebugActiveProcessStop(dwParentPid);
                return false;
            }
        }
    }
    // ...

    return 0;
}


DWORD GetParentProcessId(DWORD dwCurrentProcessId)
{
    DWORD dwParentProcessId = -1;
    PROCESSENTRY32W ProcessEntry = { 0 };
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32FirstW(hSnapshot, &ProcessEntry))
    {
        do
        {
            if (ProcessEntry.th32ProcessID == dwCurrentProcessId)
            {
                dwParentProcessId = ProcessEntry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &ProcessEntry));
    }

    CloseHandle(hSnapshot);
    return dwParentProcessId;
}

bool TestCreateToolhelp32Snapshot()
{
    bool bDebugged = false;
    DWORD dwParentProcessId = GetParentProcessId(GetCurrentProcessId());

    PROCESSENTRY32 ProcessEntry = { 0 };
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(hSnapshot, &ProcessEntry))
    {
        do
        {
            if ((ProcessEntry.th32ProcessID == dwParentProcessId) &&
                (wcscmp(ProcessEntry.szExeFile, L"explorer.exe")))
            {
                bDebugged = true;
                break;
            }
        } while (Process32Next(hSnapshot, &ProcessEntry));
    }

    CloseHandle(hSnapshot);
    return bDebugged;
}

bool Patch_DbgBreakPoint()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
        return false;

    FARPROC pDbgBreakPoint = GetProcAddress(hNtdll, "DbgBreakPoint");
    if (!pDbgBreakPoint)
        return false;

    DWORD dwOldProtect;
    if (!VirtualProtect(pDbgBreakPoint, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return false;

    *(PBYTE)pDbgBreakPoint = (BYTE)0xC3; // ret
    return true;
}


#pragma pack(push, 1)
struct DbgPatch
{
    WORD  push_0;
    BYTE  push_min1;
    DWORD CurrentProcess;
    BYTE  mov_eax;
    DWORD TerminateProcess;
    WORD  call_eax;
};
#pragma pack(pop)

bool Patch_DbgUiRemoteBreakin()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
        return false;

    FARPROC pDbgUiRemoteBreakin = GetProcAddress(hNtdll, "DbgUiRemoteBreakin");
    if (!pDbgUiRemoteBreakin)
        return false;

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32)
        return false;

    FARPROC TermProc = GetProcAddress(hKernel32, "TerminateProcess");
    if (!TermProc)
        return false;

    DbgPatch patch = { 0 };
    patch.push_0 = 0x6A00;
    patch.push_min1 = 0x68;
    patch.CurrentProcess = (DWORD)GetCurrentProcess;
    patch.mov_eax = 0xB8;
    patch.TerminateProcess = (DWORD)TermProc;
    patch.call_eax = 0xFFD0;

    DWORD dwOldProtect;
    if (!VirtualProtect(pDbgUiRemoteBreakin, sizeof(DbgPatch), PAGE_READWRITE, &dwOldProtect))
        return false;

    ::memcpy_s(pDbgUiRemoteBreakin, sizeof(DbgPatch),
        &patch, sizeof(DbgPatch));
    VirtualProtect(pDbgUiRemoteBreakin, sizeof(DbgPatch), dwOldProtect, &dwOldProtect);
    return true;
}

bool TestSwitch()
{
    HDESK hNewDesktop = CreateDesktopA(
        "desktop",
        NULL,
        NULL,
        0,
        DESKTOP_CREATEWINDOW | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP,
        NULL);
    if (!hNewDesktop)
        return false;

    return SwitchDesktop(hNewDesktop);
}

#define NtCurrentThread ((HANDLE)-2)

typedef NTSTATUS(*_NtSetInformationThread)(HANDLE, ULONG, PULONG, ULONG);
_NtSetInformationThread NtSetInformationThread;

typedef NTSTATUS(*_NtQueryInformationThread)(HANDLE, ULONG, PVOID, ULONG, PULONG); 
_NtQueryInformationThread NtQueryInformationThread;
bool was = false;
bool HideThread()
{
    if (was) return false;
    was = true;
    bool res = false;
    NTSTATUS status;
    ULONG check;
    HMODULE ntDll = LoadLibrary(L"ntdll.dll");
    NtSetInformationThread = (_NtSetInformationThread)GetProcAddress(ntDll, "NtSetInformationThread");
    NtQueryInformationThread = (_NtQueryInformationThread)GetProcAddress(ntDll, "NtQueryInformationThread");

    status = NtSetInformationThread(NtCurrentThread, 0x11, NULL, 0);
    if (res) return true;

    if (status >= 0) {
        status = NtQueryInformationThread(NtCurrentThread, 0x11, &check, sizeof(BOOLEAN), 0);
        if (status >= 0)
            return check == 0;
    }
    else
        return true;

    return false;
}


wchar_t* __stdcall FindSubstringW(const wchar_t* str, int nLength, const wchar_t* strSearch)
{
    wchar_t* cp = (wchar_t*)str;
    wchar_t* s1, * s2;
    if (!*strSearch)
        return ((wchar_t*)str);
    while (nLength && *cp)
    {
        s1 = cp;
        s2 = (wchar_t*)strSearch;
        while (*s1 && *s2 && !(*s1 - *s2))
            s1++, s2++;
        if (!*s2)
            return(cp);
        cp++;
        nLength--;
    }

    return(NULL);
}

TCHAR* ToLower(TCHAR* str, int length) {
    std::transform(
        str, str + length,
        str,
        towlower);
    return str;
}



DWORD g_dwDebuggerProcessId = -1;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD dwProcessId = *(PDWORD)lParam;

    DWORD dwWindowProcessId;

    HANDLE Handle = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        dwProcessId
    );
    if (Handle)
    {
        TCHAR Buffer[MAX_PATH];
        if (GetModuleFileNameEx(Handle, 0, Buffer, MAX_PATH))
        {
            ToLower(Buffer, wcslen(Buffer));
            if (FindSubstringW(Buffer, wcslen(Buffer), L"dbg") ||
                FindSubstringW(Buffer, wcslen(Buffer), L"debugger") ||
                FindSubstringW(Buffer, wcslen(Buffer), L"debug"))
            {
                g_dwDebuggerProcessId = dwProcessId;
                return FALSE;
            }
        }
        else
        {
            return TRUE;
        }
        CloseHandle(Handle);
    }
    return TRUE;
}

bool IsDebuggerProcess(DWORD dwProcessId)
{
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&dwProcessId));
    return g_dwDebuggerProcessId == dwProcessId;
}

bool SuspendDebuggerThread()
{
    THREADENTRY32 entry = { 0 };
    entry.dwSize = sizeof(THREADENTRY32);

    DWORD ppid = GetParentProcessId(GetCurrentProcessId());
    if (-1 == ppid)
        return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (Thread32First(hSnapshot, &entry))
    {
        do
        {
            if ((entry.th32OwnerProcessID == ppid) && IsDebuggerProcess(ppid))
            {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
                if (hThread) {
                    DWORD res = SuspendThread(hThread);
                    res += 1;
                }
                break;
            }
        } while (Thread32Next(hSnapshot, &entry));
    }

    if (hSnapshot)
        CloseHandle(hSnapshot);

    return false;
}

bool TestBlockInput()
{
    bool a = false, b = false;
    __try
    {
        a = BlockInput(TRUE);
        b = BlockInput(TRUE);
    }
    __finally
    {
        BlockInput(FALSE);
    }
    return b && a;
}