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
#include <Wbemidl.h>
#include <Psapi.h>

typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef struct _DEBUG_BUFFER {
	HANDLE SectionHandle;
	PVOID  SectionBase;
	PVOID  RemoteSectionBase;
	ULONG  SectionBaseDelta;
	HANDLE  EventPairHandle;
	ULONG  Unknown[2];
	HANDLE  RemoteThreadHandle;
	ULONG  InfoClassMask;
	ULONG  SizeOfInfo;
	ULONG  AllocatedSize;
	ULONG  SectionSize;
	PVOID  ModuleInformation;
	PVOID  BackTraceInformation;
	PVOID  HeapInformation;
	PVOID  LockInformation;
	PVOID  Reserved[8];
} DEBUG_BUFFER, * PDEBUG_BUFFER;

typedef struct _DEBUG_HEAP_INFORMATION
{
	ULONG Base; // 0x00
	ULONG Flags; // 0x04
	USHORT Granularity; // 0x08
	USHORT Unknown; // 0x0A
	ULONG Allocated; // 0x0C
	ULONG Committed; // 0x10
	ULONG TagCount; // 0x14
	ULONG BlockCount; // 0x18
	ULONG Reserved[7]; // 0x1C
	PVOID Tags; // 0x38
	PVOID Blocks; // 0x3C
} DEBUG_HEAP_INFORMATION, * PDEBUG_HEAP_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION
{
	BOOLEAN KernelDebuggerEnabled;
	BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, * PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef struct _RTL_HEAP_INFORMATION
{
	PVOID BaseAddress;
	ULONG Flags;
	USHORT EntryOverhead;
	USHORT CreatorBackTraceIndex;
	ULONG BytesAllocated;
	ULONG BytesCommitted;
	ULONG NumberOfTags;
	ULONG NumberOfEntries;
	ULONG NumberOfPseudoTags;
	ULONG PseudoTagGranularity;
	ULONG Reserved[4];
	PVOID Tags;
	PVOID Entries;
} RTL_HEAP_INFORMATION, * PRTL_HEAP_INFORMATION;

typedef struct _RTL_PROCESS_HEAPS
{
	ULONG NumberOfHeaps;
	RTL_HEAP_INFORMATION Heaps[1];
} RTL_PROCESS_HEAPS, * PRTL_PROCESS_HEAPS;

// RtlQueryProcessDebugInformation.DebugInfoClassMask constants
#define PDI_MODULES                       0x01
#define PDI_BACKTRACE                     0x02
#define PDI_HEAPS                         0x04
#define PDI_HEAP_TAGS                     0x08
#define PDI_HEAP_BLOCKS                   0x10
#define PDI_LOCKS                         0x20