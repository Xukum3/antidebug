#include "Flags.h"
#include "defs.h"
#include <VersionHelpers.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <IPTypes.h>
#include <Shlwapi.h>
#include <winternl.h>
#include <Iphlpapi.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <Wbemidl.h>
#include <Psapi.h>
#define WIN32_LEAN_AND_MEAN

bool TestIsDebuggerPresent() {
    char result = 0;
    __asm {
        mov eax, fs: [30h]
        mov al, BYTE PTR[eax + 2]
        mov result, al
    }

    return result != 0;
}

bool TestCheckRemoteDebuggerPresent() {
    BOOL bDebuggerPresent = false;
    if (TRUE == CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebuggerPresent) && TRUE == bDebuggerPresent)
        return true;
    else
        return false;
}

bool TestProcessDebugPort()
{
    // Function Pointer Typedef for NtQueryInformationProcess
    typedef NTSTATUS(WINAPI* pNtQueryInformationProcess)(IN  HANDLE, IN  UINT, OUT PVOID, IN ULONG, OUT PULONG);

    // We have to import the function
    pNtQueryInformationProcess NtQueryInfoProcess = NULL;

    // ProcessDebugPort
    const int ProcessDbgPort = 7;

    // Other Vars
    NTSTATUS Status;

    DWORD dProcessInformationLength = sizeof(ULONG) * 2;
    DWORD64 IsRemotePresent = 0;

#if defined(ENV32BIT)
    DWORD dProcessInformationLength = sizeof(ULONG);
    DWORD32 IsRemotePresent = 0;
#endif

    HMODULE hNtdll = LoadLibrary(_T("ntdll.dll"));
    if (hNtdll == NULL)
    {
        // Handle however.. chances of this failing
        // is essentially 0 however since
        // ntdll.dll is a vital system resource
    }

    NtQueryInfoProcess = (pNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

    // Sanity check although there's no reason for it to have failed
    if (NtQueryInfoProcess == NULL)
        return 0;

    // Time to finally make the call
    Status = NtQueryInfoProcess(GetCurrentProcess(), ProcessDbgPort, &IsRemotePresent, dProcessInformationLength, NULL);
    if (Status == 0x00000000 && IsRemotePresent != 0)
        return TRUE;
    else
        return FALSE;
}

bool TestProcessDebugFlags()
{
    // ProcessDebugFlags
    typedef NTSTATUS(WINAPI* pNtQueryInformationProcess)(IN  HANDLE, IN  UINT, OUT PVOID, IN ULONG, OUT PULONG);
    pNtQueryInformationProcess NtQueryInfoProcess = NULL;

    const int ProcessDebugFlags = 0x1f;

    HMODULE hNtdll = LoadLibrary(_T("ntdll.dll"));
    if (hNtdll == NULL)
    {
        // Handle however.. chances of this failing
        // is essentially 0 however since
        // ntdll.dll is a vital system resource
    }

    NtQueryInfoProcess = (pNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

    // Other Vars
    NTSTATUS Status;
    DWORD NoDebugInherit = 0;

    Status = NtQueryInfoProcess(GetCurrentProcess(), ProcessDebugFlags, &NoDebugInherit, sizeof(DWORD), NULL);
    if (Status == 0x00000000 && NoDebugInherit == 0)
        return TRUE;
    else
        return FALSE;
}


bool TestRtlQuery()
{
    NTSTATUS ntStatus;

    typedef NTSTATUS(WINAPI* pRtlQueryProcessDebugInformation)(IN ULONG, IN ULONG, IN OUT PDEBUG_BUFFER);
    typedef PDEBUG_BUFFER(WINAPI* pRtlCreateQueryDebugBuffer)(IN ULONG, IN BOOLEAN);
    typedef NTSTATUS(WINAPI* pRtlDestroyQueryDebugBuffer)(IN PDEBUG_BUFFER);

        
    pRtlQueryProcessDebugInformation RtlQueryProcessDebugInformation = NULL;
    pRtlCreateQueryDebugBuffer RtlCreateQueryDebugBuffer = NULL;
    pRtlDestroyQueryDebugBuffer RtlDestroyQueryDebugBuffer = NULL;

    HMODULE hNtdll = LoadLibraryA("ntdll.dll");

    RtlQueryProcessDebugInformation = (pRtlQueryProcessDebugInformation)GetProcAddress(hNtdll, "NtQueryInformationProcess");
    RtlCreateQueryDebugBuffer = (pRtlCreateQueryDebugBuffer)GetProcAddress(hNtdll, "NtQueryInformationProcess");

    PDEBUG_BUFFER pDebugBuffer = RtlCreateQueryDebugBuffer(0, FALSE);

    ntStatus = RtlQueryProcessDebugInformation(GetCurrentProcessId(),
        PDI_HEAPS | PDI_HEAP_BLOCKS,
        pDebugBuffer);

    ULONG dwFlags = ((PRTL_PROCESS_HEAPS)pDebugBuffer->HeapInformation)->Heaps[0].Flags;
    return dwFlags & ~HEAP_GROWABLE;
    return true;
}


bool TestSystemKernelDebuggerInformation()
{
    // SystemKernelDebuggerInformation
    const int SystemKernelDebuggerInformation = 0x23;

    // The debugger information struct
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdDebuggerInfo;

    typedef NTSTATUS(WINAPI* pNtQuerySystemInformation)(IN UINT, OUT PVOID, IN ULONG, OUT PULONG);
    pNtQuerySystemInformation NtQuerySystemInformation = NULL;

    HMODULE hNtdll = LoadLibraryA("ntdll.dll");

    NtQuerySystemInformation = (pNtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");

    // Call NtQuerySystemInformation
    NTSTATUS Status = NtQuerySystemInformation(SystemKernelDebuggerInformation, &KdDebuggerInfo, sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION), NULL);
    if (Status >= 0)
    {
        // KernelDebuggerEnabled almost always implies !KernelDebuggerNotPresent. KernelDebuggerNotPresent can sometimes
        // change if the debugger is temporarily disconnected, but either of these means a debugger is enabled.
        if (KdDebuggerInfo.KernelDebuggerEnabled || !KdDebuggerInfo.KernelDebuggerNotPresent)
            return TRUE;
    }
    return FALSE;
}




bool TestNtGlobalFlag( ) {
    PDWORD pNtGlobalFlag = NULL, pNtGlobalFlagWoW64 = NULL;

#if defined (ENV64BIT)
    pNtGlobalFlag = (PDWORD)(__readgsqword(0x60) + 0xBC);

#elif defined(ENV32BIT)
    /* NtGlobalFlags for real 32-bits OS */
    BYTE* _teb32 = (BYTE*)__readfsdword(0x18);
    DWORD _peb32 = *(DWORD*)(_teb32 + 0x30);
    pNtGlobalFlag = (PDWORD)(_peb32 + 0x68);

    if (IsWoW64())
    {
        /* In Wow64, there is a separate PEB for the 32-bit portion and the 64-bit portion
        which we can double-check */

        BYTE* _teb64 = (BYTE*)__readfsdword(0x18) - 0x2000;
        DWORD64 _peb64 = *(DWORD64*)(_teb64 + 0x60);
        pNtGlobalFlagWoW64 = (PDWORD)(_peb64 + 0xBC);
    }
#endif

    BOOL normalDetected = pNtGlobalFlag && *pNtGlobalFlag & 0x00000070;
    BOOL wow64Detected = pNtGlobalFlagWoW64 && *pNtGlobalFlagWoW64 & 0x00000070;

    if (normalDetected || wow64Detected)
        return TRUE;
    else
        return FALSE;
}

bool TestHeapCheck() {
    PROCESS_HEAP_ENTRY HeapEntry = { 0 };
    do
    {
        if (!HeapWalk(GetProcessHeap(), &HeapEntry))
            return false;
    } while (HeapEntry.wFlags != PROCESS_HEAP_ENTRY_BUSY);

    PVOID pOverlapped = (PBYTE)HeapEntry.lpData + HeapEntry.cbData;
    return ((DWORD)(*(PDWORD)pOverlapped) == 0xABABABAB);
}