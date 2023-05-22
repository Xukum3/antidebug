#include "Assembly.h"
#include "HwBrk.h"

bool TestInt3() {
    __try
    {
        __asm int 3;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TestInt2D() {
    __try
    {   
        __asm {
            xor eax, eax;
            int 0x2d;
            nop;
        }
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TestDebugBreak() {
    __try
    {
        DebugBreak();
    }
    __except (EXCEPTION_BREAKPOINT)
    {
        return false;
    }

    return true;
}

bool TestICE()  {
    __try
    {
        __asm __emit 0xF1;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TestTrapFlag() {
    bool res = false;

    __asm
    {
        push ss
        pop ss
        pushf
        test byte ptr[esp + 1], 1
        jz trap_jump
    }

    res = true;

trap_jump:
    // restore stack
    __asm popf;

    return res;
}


#include "hwbrk.h"

static LONG WINAPI InstructionCountingExeptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
    if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        pExceptionInfo->ContextRecord->Eax += 1;
        pExceptionInfo->ContextRecord->Eip += 1;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

__declspec(naked) DWORD WINAPI InstructionCountingFunc(LPVOID lpThreadParameter)
{
    __asm
    {
        xor eax, eax
        nop
        nop
        nop
        nop
        cmp al, 4
        jne being_debugged
    }

    ExitThread(FALSE);

being_debugged:
    ExitThread(TRUE);
}

HANDLE m_hHwBps[7];

bool TestInstructionCounting()
{
    PVOID hVeh = nullptr;
    HANDLE hThread = nullptr;
    bool bDebugged = false;

    int m_nInstructionCount = 4;
    __try
    {
        hVeh = AddVectoredExceptionHandler(TRUE, InstructionCountingExeptionHandler);
        if (!hVeh)
            __leave;

        hThread = CreateThread(0, 0, InstructionCountingFunc, NULL, CREATE_SUSPENDED, 0);
        if (!hThread)
            __leave;

        PVOID pThreadAddr = &InstructionCountingFunc;
        if (*(PBYTE)pThreadAddr == 0xE9)
            pThreadAddr = (PVOID)((DWORD)pThreadAddr + 5 + *(PDWORD)((PBYTE)pThreadAddr + 1));


        for (auto i = 0; i < m_nInstructionCount; i++)
            m_hHwBps[i] = SetHardwareBreakpoint(
                hThread, HWBRK_TYPE_CODE, HWBRK_SIZE_1, (PVOID)((DWORD)pThreadAddr + 2 + i));

        ResumeThread(hThread);
        WaitForSingleObject(hThread, INFINITE);

        DWORD dwThreadExitCode;
        if (TRUE == GetExitCodeThread(hThread, &dwThreadExitCode))
            bDebugged = (TRUE == dwThreadExitCode);
    }
    __finally
    {
        if (hThread)
            CloseHandle(hThread);

        for (int i = 0; i < 4; i++)
        {
            if (m_hHwBps[i])
                RemoveHardwareBreakpoint(m_hHwBps[i]);
        }

        if (hVeh)
            RemoveVectoredExceptionHandler(hVeh);
    }

    return bDebugged;
}


bool TestPopfTrap()
{
    __try
    {
        __asm
        {
            pushfd
            mov dword ptr[esp], 0x100
            popfd
            nop
        }
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_SINGLE_STEP
        ? EXCEPTION_EXECUTE_HANDLER
        : EXCEPTION_CONTINUE_EXECUTION)
    {
        return false;
    }
}

bool TestInstrPref() {
    __try
    {
        __asm __emit 0xF3
        __asm __emit 0x64
        __asm __emit 0xF1
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

bool TestSelector()
{
    __asm
    {
        push 3
        pop  gs

        __asm SeclectorsLbl:
        mov  ax, gs
            cmp  al, 3
            je   SeclectorsLbl

            push 3
            pop  gs
            mov  ax, gs
            cmp  al, 3
            jne  Selectors_Debugged
    }

    return false;

Selectors_Debugged:
    return true;
}
