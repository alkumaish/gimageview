/*
 * kernel32.c -- implementation of routines in kernel32.dll
 * (C)Copyright 2000, 2001 by Hiroshi Takekawa
 * This file is part of Enfle.
 *
 * Last Modified: Mon Feb 18 03:19:51 2002.
 * $Id$
 *
 * Enfle is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Enfle is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#define MORE_DEBUG

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#define W32API_REQUEST_MEM_ALLOC
#define W32API_REQUEST_MEM_REALLOC
#define W32API_REQUEST_MEM_FREE
#include "mm.h"
#include "w32api.h"
#include "module.h"

#include "kernel32.h"

#define REQUIRE_UNISTD_H
#define REQUIRE_STRING_H
#include "compat.h"

#include "common.h"

#ifdef USE_PTHREAD
#  include <pthread.h>
#endif

#ifdef MORE_DEBUG
#define more_debug_message(format, args...)
#define more_debug_message_fn(format, args...)
#else
#define more_debug_message(format, args...) fprintf(stderr, format, ## args)
#define more_debug_message_fn(format, args...) fprintf(stderr, "%s", __FUNCTION__);fprintf(stderr, format, ## args)
#endif

/* file related */
DECLARE_W32API(HANDLE, CreateFileA, (LPCSTR,  DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE));
DECLARE_W32API(HANDLE, CreateFileW, (LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE));
DECLARE_W32API(HFILE, _lcreat, (LPCSTR, INT));
DECLARE_W32API(HFILE, _lopen, (LPCSTR, INT));
DECLARE_W32API(BOOL, ReadFile, (HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED));
DECLARE_W32API(UINT, _lread, (HFILE, LPVOID, UINT));
DECLARE_W32API(BOOL, WriteFile, (HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED));
DECLARE_W32API(UINT, _lwrite, (HFILE, LPCSTR, UINT));
DECLARE_W32API(UINT, _lclose, (HFILE));
DECLARE_W32API(DWORD, SetFilePointer, (HANDLE, LONG, LONG *, DWORD));
DECLARE_W32API(LONG, _llseek, (HFILE, LONG, INT));
DECLARE_W32API(DWORD, GetFileSize, (HANDLE, LPDWORD));
DECLARE_W32API(DWORD, GetFileType, (HANDLE));
DECLARE_W32API(BOOL, DeleteFileA, (LPCSTR));
DECLARE_W32API(BOOL, FlushFileBuffers, (HANDLE));
DECLARE_W32API(LPVOID, MapViewOfFile, (HANDLE, DWORD, DWORD, DWORD, DWORD));
DECLARE_W32API(LPVOID, MapViewOfFileEx, (HANDLE, DWORD, DWORD, DWORD, DWORD, LPVOID));
DECLARE_W32API(BOOL, UnmapViewOfFile, (LPVOID));
DECLARE_W32API(HANDLE, CreateFileMappingA, (HANDLE, SECURITY_ATTRIBUTES *, DWORD, DWORD, DWORD, LPCSTR));
DECLARE_W32API(BOOL, SetEndOfFile, (HANDLE));
/* directory related */
DECLARE_W32API(BOOL, CreateDirectoryA, (LPCSTR, LPSECURITY_ATTRIBUTES));
DECLARE_W32API(BOOL, CreateDirectoryW, (LPCWSTR, LPSECURITY_ATTRIBUTES));
/* handle related */
DECLARE_W32API(HANDLE, GetStdHandle, (DWORD));
DECLARE_W32API(BOOL, SetStdHandle, (DWORD, HANDLE));
DECLARE_W32API(UINT, SetHandleCount, (UINT));
DECLARE_W32API(BOOL, CloseHandle, (HANDLE));
/* module related */
DECLARE_W32API(HMODULE, LoadLibraryA, (LPCSTR));
DECLARE_W32API(HMODULE, LoadLibraryExA, (LPCSTR, HANDLE, DWORD));
DECLARE_W32API(HMODULE, GetModuleHandleA, (LPCSTR));
DECLARE_W32API(DWORD, GetModuleFileNameA, (HMODULE, LPSTR, DWORD));
/* memory related */
DECLARE_W32API(HLOCAL, LocalAlloc, (UINT, DWORD));
DECLARE_W32API(HLOCAL, LocalReAlloc, (HLOCAL, DWORD, UINT));
DECLARE_W32API(HLOCAL, LocalFree, (HLOCAL));
DECLARE_W32API(LPVOID, LocalLock, (HLOCAL));
DECLARE_W32API(BOOL, LocalUnlock, (HLOCAL));
DECLARE_W32API(HGLOBAL, GlobalAlloc, (UINT, DWORD));
DECLARE_W32API(HGLOBAL, GlobalReAlloc, (HGLOBAL, DWORD, UINT));
DECLARE_W32API(HGLOBAL, GlobalFree, (HGLOBAL));
DECLARE_W32API(LPVOID, GlobalLock, (HGLOBAL));
DECLARE_W32API(BOOL, GlobalUnlock, (HGLOBAL));
DECLARE_W32API(VOID, GlobalMemoryStatus, (LPMEMORYSTATUS));
DECLARE_W32API(LPVOID, VirtualAlloc, (LPVOID, DWORD, DWORD, DWORD));
DECLARE_W32API(BOOL, VirtualFree, (LPVOID, DWORD, DWORD));
DECLARE_W32API(DWORD, VirtualQuery, (LPCVOID, LPMEMORY_BASIC_INFORMATION, DWORD));
DECLARE_W32API(BOOL, IsBadReadPtr, (LPCVOID, UINT));
DECLARE_W32API(BOOL, IsBadWritePtr, (LPCVOID, UINT));
DECLARE_W32API(BOOL, IsBadCodePtr, (FARPROC));
/* resource related */
DECLARE_W32API(HRSRC, FindResourceA, (HMODULE, LPCSTR, LPCSTR));
DECLARE_W32API(DWORD, SizeofResource, (HMODULE, HRSRC));
/* heap related */
DECLARE_W32API(HANDLE, HeapCreate, (DWORD, DWORD, DWORD));
DECLARE_W32API(LPVOID, HeapAlloc, (HANDLE, DWORD, DWORD));
DECLARE_W32API(LPVOID, HeapReAlloc, (HANDLE, DWORD, LPVOID, DWORD));
DECLARE_W32API(BOOL, HeapFree, (HANDLE, DWORD, LPVOID));
DECLARE_W32API(BOOL, HeapDestroy, (HANDLE));
DECLARE_W32API(HANDLE, GetProcessHeap, (void));
DECLARE_W32API(DWORD, GetProcessHeaps, (DWORD, HANDLE *));
/* string related */
DECLARE_W32API(INT, lstrlenA, (LPCSTR));
DECLARE_W32API(LPSTR, lstrcpyA, (LPSTR, LPCSTR));
DECLARE_W32API(LPSTR, lstrcatA, (LPSTR, LPCSTR));
DECLARE_W32API(INT, lstrcmpA, (LPCSTR, LPCSTR));
DECLARE_W32API(INT, lstrcmpiA, (LPCSTR, LPCSTR));
/* process related */
DECLARE_W32API(HANDLE, GetCurrentProcess, (void));
DECLARE_W32API(FARPROC, GetProcAddress, (HMODULE, LPCSTR));
DECLARE_W32API(void, ExitProcess, (DWORD)) __attribute__ ((noreturn));
DECLARE_W32API(BOOL, TerminateProcess, (HANDLE, DWORD));
/* thread related */
DECLARE_W32API(HANDLE, GetCurrentThread, (void));
DECLARE_W32API(DWORD, GetCurrentThreadId, (void));
DECLARE_W32API(LCID, GetThreadLocale, (void));
DECLARE_W32API(BOOL, DisableThreadLibraryCalls, (HMODULE));
DECLARE_W32API(HANDLE, CreateThread, (SECURITY_ATTRIBUTES *, DWORD, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD));
/* thread local-variable related */
DECLARE_W32API(DWORD, TlsAlloc, (void));
DECLARE_W32API(BOOL, TlsFree, (DWORD));
DECLARE_W32API(LPVOID, TlsGetValue, (DWORD));
DECLARE_W32API(BOOL, TlsSetValue, (DWORD, LPVOID));
/* critical section */
DECLARE_W32API(void, InitializeCriticalSection, (CRITICAL_SECTION *));
DECLARE_W32API(void, EnterCriticalSection, (CRITICAL_SECTION *));
DECLARE_W32API(void, LeaveCriticalSection, (CRITICAL_SECTION *));
DECLARE_W32API(void, DeleteCriticalSection, (CRITICAL_SECTION *));
/* interlocked */
DECLARE_W32API(LONG, InterlockedExchangeAdd, (PLONG,LONG));
DECLARE_W32API(LONG, InterlockedIncrement, (PLONG));
DECLARE_W32API(LONG, InterlockedDecrement, (PLONG));
/* exception */
DECLARE_W32API(DWORD, UnhandledExceptionFilter, (PEXCEPTION_POINTERS));
DECLARE_W32API(LPTOP_LEVEL_EXCEPTION_FILTER, SetUnhandledExceptionFilter, (LPTOP_LEVEL_EXCEPTION_FILTER));
DECLARE_W32API(void, RaiseException, (DWORD, DWORD, DWORD, const LPDWORD));
DECLARE_W32API(void, RtlUnwind, (PEXCEPTION_FRAME, LPVOID, PEXCEPTION_RECORD, DWORD));
/* Environment */
DECLARE_W32API(LPSTR, GetCommandLineA, (void));
DECLARE_W32API(DWORD, GetEnvironmentVariableA, (LPCSTR,LPSTR,DWORD));
DECLARE_W32API(LPSTR, GetEnvironmentStringsA, (void));
DECLARE_W32API(LPWSTR, GetEnvironmentStringsW, (void));
DECLARE_W32API(BOOL, FreeEnvironmentStringsA, (LPSTR));
DECLARE_W32API(BOOL, FreeEnvironmentStringsW, (LPWSTR));
DECLARE_W32API(VOID, GetStartupInfoA, (LPSTARTUPINFOA));
/* codepage */
DECLARE_W32API(INT, GetLocaleInfoA, (LCID, LCTYPE, LPSTR, INT));
DECLARE_W32API(UINT, GetACP, (void));
DECLARE_W32API(UINT, GetOEMCP, (void));
DECLARE_W32API(BOOL, GetCPInfo, (UINT, LPCPINFO));
DECLARE_W32API(BOOL, GetStringTypeA, (LCID, DWORD, LPCSTR, INT, LPWORD));
DECLARE_W32API(BOOL, GetStringTypeW, (DWORD, LPCWSTR, INT, LPWORD));
DECLARE_W32API(INT, MultiByteToWideChar, (UINT, DWORD, LPCSTR, INT, LPWSTR, INT));
DECLARE_W32API(INT, WideCharToMultiByte, (UINT, DWORD, LPCWSTR, INT, LPSTR, INT, LPCSTR, BOOL *));
DECLARE_W32API(INT, LCMapStringA, (LCID, DWORD, LPCSTR, INT, LPSTR, INT));
DECLARE_W32API(BOOL, IsDBCSLeadByte, (BYTE));
/* date and time */
DECLARE_W32API(BOOL, EnumCalendarInfoA, (CALINFO_ENUMPROCA, LCID, CALID, CALTYPE));
DECLARE_W32API(VOID, GetLocalTime, (LPSYSTEMTIME));
/* miscellaneous */
DECLARE_W32API(DWORD, GetLastError, (void));
DECLARE_W32API(void, SetLastError, (DWORD));
DECLARE_W32API(LONG, GetVersion, (void));
DECLARE_W32API(BOOL, GetVersionExA, (OSVERSIONINFOA *));
/* unimplemented */
DECLARE_W32API(void, unknown_symbol, (void));

static Symbol_info symbol_infos[] = {
  { "CreateFileA", CreateFileA },
  { "CreateFileW", CreateFileW },
  { "_lcreat", _lcreat },
  { "_lopen", _lopen },
  { "ReadFile", ReadFile },
  { "_lread", _lread },
  { "WriteFile", WriteFile },
  { "_lwrite", _lwrite },
  { "_lclose", _lclose },
  { "SetFilePointer", SetFilePointer },
  { "_llseek", _llseek },
  { "GetFileSize", GetFileSize },
  { "GetFileType", GetFileType },
  { "DeleteFileA", DeleteFileA },
  { "FlushFileBuffers", FlushFileBuffers },
  { "MapViewOfFile", MapViewOfFile },
  { "MapViewOfFileEx", MapViewOfFileEx },
  { "UnmapViewOfFile", UnmapViewOfFile },
  { "CreateFileMappingA", CreateFileMappingA },
  { "SetEndOfFile", SetEndOfFile },
  { "CreateDirectoryA", CreateDirectoryA },
  { "CreateDirectoryW", CreateDirectoryW },
  { "GetStdHandle", GetStdHandle },
  { "SetStdHandle", SetStdHandle },
  { "SetHandleCount", SetHandleCount },
  { "CloseHandle", CloseHandle },
  { "LoadLibraryA", LoadLibraryA },
  { "LoadLibraryExA", LoadLibraryExA },
  { "GetModuleHandleA", GetModuleHandleA },
  { "GetModuleFileNameA", GetModuleFileNameA },
  { "LocalAlloc", LocalAlloc },
  { "LocalReAlloc", LocalReAlloc },
  { "LocalFree", LocalFree },
  { "LocalLock", LocalLock },
  { "LocalUnlock", LocalUnlock },
  { "GlobalAlloc", GlobalAlloc },
  { "GlobalReAlloc", GlobalReAlloc },
  { "GlobalFree", GlobalFree },
  { "GlobalLock", GlobalLock },
  { "GlobalUnlock", GlobalUnlock },
  { "GlobalMemoryStatus", GlobalMemoryStatus },
  { "VirtualAlloc", VirtualAlloc },
  { "VirtualFree", VirtualFree },
  { "VirtualQuery", VirtualQuery },
  { "IsBadReadPtr", IsBadReadPtr },
  { "IsBadWritePtr", IsBadWritePtr },
  { "IsBadCodePtr", IsBadCodePtr },
  { "FindResourceA", FindResourceA },
  { "SizeofResource", SizeofResource },
  { "HeapCreate", HeapCreate },
  { "HeapAlloc", HeapAlloc },
  { "HeapReAlloc", HeapReAlloc },
  { "HeapFree", HeapFree },
  { "HeapDestroy", HeapDestroy },
  { "GetProcessHeap", GetProcessHeap },
  { "GetProcessHeaps", GetProcessHeaps },
  { "lstrlenA", lstrlenA },
  { "lstrcpyA", lstrcpyA },
  { "lstrcatA", lstrcatA },
  { "lstrcmpA", lstrcmpA },
  { "lstrcmpiA", lstrcmpiA },
  { "GetCurrentProcess", GetCurrentProcess },
  { "GetProcAddress", GetProcAddress },
  { "ExitProcess", ExitProcess },
  { "TerminateProcess", TerminateProcess },
  { "GetCurrentThread", GetCurrentThread },
  { "GetCurrentThreadId", GetCurrentThreadId },
  { "GetThreadLocale", GetThreadLocale },
  { "DisableThreadLibraryCalls", DisableThreadLibraryCalls },
  { "CreateThread", CreateThread },
  { "TlsAlloc", TlsAlloc },
  { "TlsFree", TlsFree },
  { "TlsGetValue", TlsGetValue },
  { "TlsSetValue", TlsSetValue },
  { "InitializeCriticalSection", InitializeCriticalSection },
  { "EnterCriticalSection", EnterCriticalSection },
  { "LeaveCriticalSection", LeaveCriticalSection },
  { "DeleteCriticalSection", DeleteCriticalSection },
  { "InterlockedExchangeAdd", InterlockedExchangeAdd },
  { "InterlockedIncrement", InterlockedIncrement },
  { "InterlockedDecrement", InterlockedDecrement },
  { "UnhandledExceptionFilter", UnhandledExceptionFilter },
  { "SetUnhandledExceptionFilter", SetUnhandledExceptionFilter },
  { "RaiseException", RaiseException },
  { "RtlUnwind", RtlUnwind },
  { "GetCommandLineA", GetCommandLineA },
  { "GetEnvironmentVariableA", GetEnvironmentVariableA },
  { "GetEnvironmentStrings", GetEnvironmentStringsA },
  { "GetEnvironmentStringsA", GetEnvironmentStringsA },
  { "GetEnvironmentStringsW", GetEnvironmentStringsW },
  { "FreeEnvironmentStringsA", FreeEnvironmentStringsA },
  { "FreeEnvironmentStringsW", FreeEnvironmentStringsW },
  { "GetStartupInfoA", GetStartupInfoA },
  { "GetLocaleInfoA", GetLocaleInfoA },
  { "GetACP", GetACP },
  { "GetOEMCP", GetOEMCP },
  { "GetCPInfo", GetCPInfo },
  { "GetStringTypeA", GetStringTypeA },
  { "GetStringTypeW", GetStringTypeW },
  { "MultiByteToWideChar", MultiByteToWideChar },
  { "WideCharToMultiByte", WideCharToMultiByte },
  { "LCMapStringA", LCMapStringA },
  { "IsDBCSLeadByte", IsDBCSLeadByte },
  { "EnumCalendarInfoA", EnumCalendarInfoA },
  { "GetLocalTime", GetLocalTime },
  { "GetLastError", GetLastError },
  { "SetLastError", SetLastError },
  { "GetVersion", GetVersion },
  { "GetVersionExA", GetVersionExA },
  { NULL, unknown_symbol }
};

/* file related */

DEFINE_W32API(HANDLE, CreateFileA,
	      (LPCSTR filename, DWORD _access, DWORD sharing,
	       LPSECURITY_ATTRIBUTES sa, DWORD creation,
	       DWORD attributes, HANDLE template))
{
  HANDLE handle;

  debug_message_fn("(%s): ", filename);
  handle = (HANDLE)fopen(filename, "rb");
  debug_message("%p\n", handle);

  return handle;
}

DEFINE_W32API(HANDLE, CreateFileW,
	      (LPCWSTR filename, DWORD _access, DWORD sharing,
	       LPSECURITY_ATTRIBUTES sa, DWORD creation, DWORD attributes, HANDLE template))
{
  HANDLE handle;

  debug_message_fn("(%s): ", (char *)filename);
  handle = (HANDLE)fopen((char *)filename, "rb");
  debug_message("%p\n", handle);

  return handle;
}

DEFINE_W32API(HFILE, _lcreat,
	      (LPCSTR path, INT mode))
{
  debug_message_fn("(%s, %d)\n", path, mode);
  return NULL;
}

DEFINE_W32API(HFILE, _lopen,
	      (LPCSTR path, INT mode))
{
  DWORD _access = 0, sharing = 0;

  return CreateFileA(path, _access, sharing, NULL, OPEN_EXISTING, 0, (HANDLE)HFILE_ERROR);
}

DEFINE_W32API(BOOL, ReadFile,
	      (HANDLE handle, LPVOID buffer, DWORD bytes_to_read,
	       LPDWORD bytes_read, LPOVERLAPPED overlapped))
{
  int _bytes_read;
  more_debug_message_fn("(%p, %p, %d bytes)\n", handle, buffer, bytes_to_read);

  if (bytes_read)
    *bytes_read = 0;
  if (bytes_to_read == 0)
    return TRUE;
  _bytes_read = fread(buffer, 1, bytes_to_read, (FILE *)handle);
  *bytes_read = (DWORD)_bytes_read;

  return (_bytes_read >= 0) ? TRUE : FALSE;
}

DEFINE_W32API(UINT, _lread,
	      (HFILE handle, LPVOID buffer, UINT count))
{
  DWORD result;

  if (!ReadFile(handle, buffer, count, &result, NULL))
    return HFILE_ERROR;
  return result;
}

DEFINE_W32API(BOOL, WriteFile,
	      (HANDLE handle, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written,
	       LPOVERLAPPED overlapped))
{
  debug_message_fn("(handle %p, buffer %p, to_write %d)\n", handle, buffer, bytes_to_write);

  //return TRUE;
  return FALSE;
}

DEFINE_W32API(UINT, _lwrite,
	      (HFILE handle, LPCSTR buffer, UINT len))
{
  debug_message_fn("(handle %p, buffer %p, to_write %d)\n", handle, buffer, len);

  return HFILE_ERROR;
}

DEFINE_W32API(UINT, _lclose,
	       (HFILE handle))
{
  debug_message_fn("(%p)\n", handle);
  fclose((FILE *)handle);
  return 0;
}

DEFINE_W32API(DWORD, SetFilePointer,
	      (HANDLE handle, LONG offset, LONG *high, DWORD whence))
{
  debug_message_fn("(%p, %ld, %d)\n", handle, offset, whence);

  if (high && *high) {
    debug_message("*high != 0: Not supported...\n");
    return 0;
  }

  if (fseek((FILE *)handle, offset, whence) == 0)
    return ftell((FILE *)handle);

  return HFILE_ERROR;
}

DEFINE_W32API(LONG, _llseek,
	      (HFILE handle, LONG offset, INT whence))
{
  return SetFilePointer(handle, offset, NULL, whence);
}

DEFINE_W32API(DWORD, GetFileSize,
	      (HANDLE handle, LPDWORD filesizehigh))
{
  DWORD size, cur;

  debug_message_fn("(%p): ", handle);
  if (filesizehigh)
    *filesizehigh = 0;
  cur = ftell((FILE *)handle);
  fseek((FILE *)handle, 0, SEEK_END);
  size = ftell((FILE *)handle);
  fseek((FILE *)handle, cur, SEEK_SET);

  debug_message("%d bytes\n", size);
  return size;
}

DEFINE_W32API(DWORD, GetFileType,
	      (HANDLE handle))
{
  debug_message_fn("(%p)\n", handle);

  return FILE_TYPE_DISK;
}

DEFINE_W32API(BOOL, DeleteFileA,
	      (LPCSTR path))
{
  debug_message_fn("(%s)\n", path);
  return FALSE;
}

DEFINE_W32API(BOOL, FlushFileBuffers,
	      (HANDLE h))
{
  debug_message_fn("(%p)\n", h);
  return TRUE;
}

/*
 HANDLE mapping:    [in] File-mapping object to map
 DWORD access:      [in] Access mode
 DWORD offset_high: [in] High-order 32 bits of file offset
 DWORD offset_low:  [in] Low-order 32 bits of file offset
 DWORD count:       [in] Number of bytes to map
*/
DEFINE_W32API(LPVOID, MapViewOfFile,
	      (HANDLE mapping, DWORD acc, DWORD high, DWORD low, DWORD count))
{
  FILE *fp = (FILE *)mapping;
  int fd = fileno(fp);
  struct stat st;
  LPVOID p;

  debug_message_fn("(%p, %d, %d, %d, %d): ", mapping, acc, high, low, count);
  if (fstat(fd, &st)) {
    debug_message("fstat() failed.\n");
    return NULL;
  }

  if ((p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
    debug_message("mmap() failed.\n");
    return NULL;
  }

  debug_message("%p\n", p);

  return p;
}

/*
 HANDLE mapping:    [in] File-mapping object to map
 DWORD access:      [in] Access mode
 DWORD offset_high: [in] High-order 32 bits of file offset
 DWORD offset_low:  [in] Low-order 32 bits of file offset
 DWORD count:       [in] Number of bytes to map
 LPVOID addr:       [in] Suggested starting address for mapped view
*/
DEFINE_W32API(LPVOID, MapViewOfFileEx,
	      (HANDLE mapping, DWORD acc, DWORD high, DWORD low, DWORD count, LPVOID addr))
{
  debug_message_fn("(%p, %d, %d, %d, %d, %p)\n", mapping, acc, high, low, count, addr);
  return NULL;
}

// LPVOID addr /* [in] Address where mapped view begins */
DEFINE_W32API(BOOL, UnmapViewOfFile,
	      (LPVOID addr))
{
  debug_message_fn("(%p)\n", addr);
  return TRUE;
}

/*
 HANDLE hFile:    [in] Handle of file to map
 SECURITY_ATTRIBUTES *sa: [in] Optional security attributes
 DWORD protect:   [in] Protection for mapping object
 DWORD size_high: [in] High-order 32 bits of object size
 DWORD size_low:  [in] Low-order 32 bits of object size
 LPCSTR name:     [in] Name of file-mapping object
*/
DEFINE_W32API(HANDLE, CreateFileMappingA,
	      (HANDLE h, SECURITY_ATTRIBUTES *sa, DWORD protect, DWORD high, DWORD low, LPCSTR name))
{
  debug_message_fn("(%p, %p, %d, %d, %d, %s)\n", h, sa, protect, high, low, name);
  return h;
}

DEFINE_W32API(BOOL, SetEndOfFile, (HANDLE h))
{
  debug_message_fn("(%p)\n", h);
  return TRUE;
}

/* directory related */

DEFINE_W32API(BOOL, CreateDirectoryA,
	      (LPCSTR path, LPSECURITY_ATTRIBUTES sa))
{
  debug_message_fn("(%s)\n", path);
  return TRUE;
}

DEFINE_W32API(BOOL, CreateDirectoryW,
	      (LPCWSTR path, LPSECURITY_ATTRIBUTES sa))
{
  debug_message_fn("(%s)\n", (char *)path);
  return TRUE;
}

/* handle related */

DEFINE_W32API(HANDLE, GetStdHandle,
	      (DWORD std))
{
  debug_message_fn("(%d): ", std);

  switch (std) {
  case STD_INPUT_HANDLE:
    debug_message("stdin returned.\n");
    return (HANDLE)stdin;
  case STD_OUTPUT_HANDLE:
    debug_message("stdout returned.\n");
    return (HANDLE)stdout;
  case STD_ERROR_HANDLE:
    debug_message("stderr returned.\n");
    return (HANDLE)stderr;
  default:
    debug_message("unknown.\n");
    break;
  }
  return (HANDLE)INVALID_HANDLE_VALUE;
}

DEFINE_W32API(BOOL, SetStdHandle,
	      (DWORD std, HANDLE h))
{
  debug_message_fn("(%d, %p)\n", std, h);
  return TRUE;
}

DEFINE_W32API(UINT, SetHandleCount,
	      (UINT c))
{
  return (c < 256) ? c : 256;
}

DEFINE_W32API(BOOL, CloseHandle,
	      (HANDLE handle))
{
  /* XXX: Nasty workaround... */
  static HANDLE prev = NULL;

  debug_message_fn("(%p)\n", handle);
  if (prev == handle)
    return TRUE;
  prev = handle;
  fclose((FILE *)handle);
  return TRUE;
}

/* module related */

DEFINE_W32API(HMODULE, LoadLibraryA,
	      (LPCSTR name))
{
  debug_message_fn("(%s)\n", name);
  return LoadLibraryExA(name, 0, 0);
}

DEFINE_W32API(HMODULE, LoadLibraryExA,
	      (LPCSTR name, HANDLE handle, DWORD flags))
{
  debug_message_fn("(%s, handle %p)\n", name, handle);
  return NULL;
}

DEFINE_W32API(HMODULE, GetModuleHandleA,
	      (LPCSTR name))
{
  debug_message_fn("(%s)\n", name);
  return module_lookup(name);
}

DEFINE_W32API(DWORD, GetModuleFileNameA,
	      (HMODULE handle, LPSTR s, DWORD size))
{
  debug_message_fn("(%p, %p[%s], %d)\n", handle, s, s, size);
  /* XXX */
  strcpy(s, "c:\\windows\\system\\enfle.dll");

  return strlen(s);
}

/* memory related */

DEFINE_W32API(HLOCAL, LocalAlloc,
	      (UINT flags, DWORD size))
{
  void *p;

  more_debug_message_fn("(0x%X, %d bytes): ", flags, size);

  p = w32api_mem_alloc(size);
  if ((flags & LMEM_ZEROINIT) && p)
    memset(p, 0, size);

  more_debug_message("%p\n", p);

  return p;
}

DEFINE_W32API(HLOCAL, LocalReAlloc,
	      (HLOCAL handle, DWORD size, UINT flags))
{
  more_debug_message_fn("(%p, %d, 0x%X)\n", handle, size, flags);
  return w32api_mem_realloc(handle, size);
}

DEFINE_W32API(HLOCAL, LocalFree,
	      (HLOCAL handle))
{
  more_debug_message_fn("(%p)\n", handle);
  w32api_mem_free(handle);
  return NULL;
}

DEFINE_W32API(LPVOID, LocalLock,
	      (HLOCAL handle))
{
  more_debug_message_fn("(%p)\n", handle);
  return handle;
}

DEFINE_W32API(BOOL, LocalUnlock,
	      (HLOCAL handle))
{
  more_debug_message_fn("(%p)\n", handle);
  return TRUE;
}

DEFINE_W32API(HGLOBAL, GlobalAlloc,
	      (UINT flags, DWORD size))
{
  void *p;

  more_debug_message_fn("(0x%X, %d bytes): ", flags, size);

  p = w32api_mem_alloc(size);
  if ((flags & GMEM_ZEROINIT) && p)
    memset(p, 0, size);

  more_debug_message("%p\n", p);

  return p;
}

DEFINE_W32API(HGLOBAL, GlobalReAlloc,
	      (HGLOBAL handle, DWORD size, UINT flags))
{
  more_debug_message_fn("(%d, %d, 0x%X)\n", (int)handle, size, flags);
  return w32api_mem_realloc(handle, size);
}

DEFINE_W32API(HGLOBAL, GlobalFree,
	      (HGLOBAL handle))
{
  more_debug_message_fn("(%p)\n", handle);
  w32api_mem_free(handle);
  return NULL;
}

DEFINE_W32API(LPVOID, GlobalLock,
	      (HGLOBAL handle))
{
  more_debug_message_fn("()\n");
  return handle;
}

DEFINE_W32API(BOOL, GlobalUnlock,
	      (HGLOBAL handle))
{
  more_debug_message_fn("(%p)\n", handle);
  return TRUE;
}

DEFINE_W32API(VOID, GlobalMemoryStatus,
	      (LPMEMORYSTATUS lpstat))
{
  more_debug_message_fn("()\n");
  /* XXX */
}

typedef struct _vm_commited VMCommited;
struct _vm_commited {
  void *address;
  unsigned int size;
  VMCommited *next;
  VMCommited *prev;
};

typedef struct _vm_reserved VMReserved;
struct _vm_reserved {
  void *base;
  unsigned int size;
  VMCommited *commited;
  VMReserved *next;
  VMReserved *prev;
};

static VMReserved *vmr = NULL;

DEFINE_W32API(LPVOID, VirtualAlloc,
	      (LPVOID ptr, DWORD size, DWORD type, DWORD protect))
{
  void *p;

  more_debug_message_fn("(%p, size %d, type 0x%X, protect %d)\n", ptr, size, type, protect);

  if (type & MEM_RESERVE) {
    p = w32api_mem_alloc(size);
    if (vmr == NULL) {
      if ((vmr = calloc(1, sizeof(VMReserved))) == NULL)
	return NULL;
    } else {
      if ((vmr->next = calloc(1, sizeof(VMReserved))) == NULL)
	return NULL;
      vmr->next->prev = vmr;
      vmr = vmr->next;
    }
    vmr->base = p;
    vmr->size = size;
    /* vmr->commited = NULL; vmr->next = vmr->prev = NULL; (calloc() do this) */

    debug_message_fnc("reserve: %p\n", p);

    return p;
  }

  if (type & MEM_COMMIT) {
    VMReserved *v;

    for (v = vmr; v; v = v->prev) {
      if (v->base <= ptr && ptr + size < v->base + v->size) {
	VMCommited *vc;

	if (v->commited) {
	  for (vc = v->commited; vc; vc = vc->prev) {
	    if (vc->address <= ptr && ptr < vc->address + vc->size)
	      return NULL;
	    if (vc->address <= ptr + size && ptr + size < vc->address + vc->size)
	      return NULL;
	  }
	  if ((v->commited->next = calloc(1, sizeof(VMCommited))) == NULL)
	    return NULL;
	  v->commited->next->prev = v->commited;
	  v->commited = v->commited->next;
	} else {
	  if ((v->commited = calloc(1, sizeof(VMCommited))) == NULL)
	    return NULL;
	}
	v->commited->address = ptr;
	v->commited->size = size;

	debug_message_fnc("commit: %p\n", ptr);

	return ptr;
      }
    }
    return NULL;
  }

  debug_message_fnc("neither MEM_RESERVE nor MEM_COMMIT\n");
  return NULL;
}

DEFINE_W32API(BOOL, VirtualFree,
	      (LPVOID ptr, DWORD size, DWORD type))
{
  more_debug_message_fn("(%p, size %d, type 0x%X)\n", ptr, size, type);

  if (type & MEM_RELEASE) {
    VMReserved *v;
    VMCommited *vc, *vc_p;

    for (v = vmr; v; v = v->prev) {
      if (v->base == ptr) {
	free(ptr);
	if (v->next)
	  v->next->prev = v->prev;
	if (v->prev)
	  v->prev->next = v->next;
	for (vc = v->commited; vc; vc = vc_p) {
	  vc_p = vc->prev;
	  free(vc);
	}
	free(v);
	return TRUE;
      }
    }
    return FALSE;
  }

  if (type & MEM_DECOMMIT) {
    VMReserved *v;

    for (v = vmr; v; v = v->prev) {
      if (v->base <= ptr && ptr + size <= v->base + v->size) {
	VMCommited *vc;

	for (vc = v->commited; vc; vc = vc->prev) {
	  if (vc->address == ptr) {
	    if (vc->next)
	      vc->next->prev = vc->prev;
	    if (vc->prev)
	      vc->prev->next = vc->next;
	    free(vc);
	    return TRUE;
	  }
	}
	return FALSE;
      }
    }
    return FALSE;
  }

  debug_message_fnc("neither MEM_RELEASE nor MEM_DECOMMIT\n");
  return FALSE;
}

DEFINE_W32API(DWORD, VirtualQuery,
	      (LPCVOID p, LPMEMORY_BASIC_INFORMATION info, DWORD len))
{
  debug_message_fn("(%p size %d)\n", p, len);
  return sizeof(*info);
}

DEFINE_W32API(BOOL, IsBadReadPtr,
	      (LPCVOID ptr, UINT size))
{
  debug_message_fn("(%p size %d)\n", ptr, size);
  return FALSE;
}

DEFINE_W32API(BOOL, IsBadWritePtr,
	      (LPCVOID ptr, UINT size))
{
  debug_message_fn("(%p size %d)\n", ptr, size);
  return FALSE;
}

DEFINE_W32API(BOOL, IsBadCodePtr,
	      (FARPROC p))
{
  debug_message_fn("(%p)\n", p);
  return FALSE;
}

/* resource related */

DEFINE_W32API(HRSRC, FindResourceA,
	      (HMODULE p, LPCSTR a, LPCSTR b))
{
  debug_message_fn("(module %p %s %s)\n", p, a, b);
  return (HRSRC)0;
}

DEFINE_W32API(DWORD, SizeofResource, (HMODULE p, HRSRC r))
{
  debug_message_fn("(module %p, HRSRC %p)\n", p, r);
  return (DWORD)0;
}

/* heap related */

DEFINE_W32API(HANDLE, HeapCreate,
	      (DWORD flags, DWORD init, DWORD max))
{
  more_debug_message_fn("(0x%X, %d, %d)\n", flags, init, max);
  if (init)
    return (HANDLE)w32api_mem_alloc(init);
  debug_message_fn("() called with init == 0\n");
  return (HANDLE)w32api_mem_alloc(100000);
}

DEFINE_W32API(LPVOID, HeapAlloc,
	      (HANDLE handle, DWORD flags, DWORD size))
{
  void *p;

  p = w32api_mem_alloc(size);

  more_debug_message_fn("(%p, 0x%X, %d) -> %p\n", handle, flags, size, p);

  if ((flags & HEAP_ZERO_MEMORY) && p)
    memset(p, 0, size);

  return p;
}

DEFINE_W32API(LPVOID, HeapReAlloc,
	      (HANDLE handle, DWORD flags, LPVOID ptr, DWORD size))
{
  more_debug_message_fn("(%p, 0x%X, %p, %d)\n", handle, flags, ptr, size);
  return w32api_mem_realloc(ptr, size);
}

DEFINE_W32API(BOOL, HeapFree,
	      (HANDLE handle, DWORD flags, LPVOID ptr))
{
  more_debug_message_fn("(%p, 0x%X, %p)\n", handle, flags, ptr);
  w32api_mem_free(ptr);

  return TRUE;
}

DEFINE_W32API(BOOL, HeapDestroy,
	      (HANDLE handle))
{
  more_debug_message_fn("(%p)\n", handle);
  w32api_mem_free(handle);
  return TRUE;
}

DEFINE_W32API(HANDLE, GetProcessHeap,
	      (void))
{
  more_debug_message_fn("()\n");
  return (HANDLE)1;
}

DEFINE_W32API(DWORD, GetProcessHeaps,
	      (DWORD count, HANDLE *heaps))
{
  debug_message_fn("(count %d, handle *%p)\n", count, heaps);
  return 1;
}

/* string related */

DEFINE_W32API(INT, lstrlenA,
	       (LPCSTR str))
{
  debug_message_fn("(%s)\n", str);

  if (str)
    return strlen(str);
  return 0;
}

DEFINE_W32API(LPSTR, lstrcpyA,
	      (LPSTR dest, LPCSTR src))
{
  /* debug_message_fn("(%p, %s)\n", dest, src); */

  memcpy(dest, src, strlen(src) + 1);

  return dest;
}

DEFINE_W32API(LPSTR, lstrcatA,
	      (LPSTR dest, LPCSTR src))
{
  debug_message_fn("(%p, %s)\n", dest, src);

  memcpy(dest + strlen(dest), src, strlen(src) + 1);

  return dest;
}

DEFINE_W32API(INT, lstrcmpA,
	      (LPCSTR a, LPCSTR b))
{
  return strcmp(a, b);
}

DEFINE_W32API(INT, lstrcmpiA,
	      (LPCSTR a, LPCSTR b))
{
  return strcasecmp(a, b);
}

/* process related */

DEFINE_W32API(HANDLE, GetCurrentProcess,
	      (void))
{
  debug_message_fn("().\n");
  return (HANDLE)0xffffffff;
}

DEFINE_W32API(FARPROC, GetProcAddress,
	     (HMODULE handle, LPCSTR funcname))
{
  Symbol_info *syminfo;
  int i;

  debug_message_fn("(%p, %s)\n", handle, funcname);

  if ((syminfo = (Symbol_info *)handle) == NULL)
    return NULL;

  /* hashing should be used */
  for (i = 0; syminfo[i].name; i++)
    if (strcmp(syminfo[i].name, funcname) == 0) {
      debug_message_fnc("resolve: %s -> %p\n", funcname, syminfo[i].value);
      return (FARPROC)syminfo[i].value;
    }

  return NULL;
}

DEFINE_W32API(void, ExitProcess, (DWORD status))
{
  debug_message_fn("()\n");
  exit(status);
}

DEFINE_W32API(BOOL, TerminateProcess,
	      (HANDLE h, DWORD d))
{
  debug_message_fn("(%p, %d)\n", h, d);
  return TRUE;
}

/* thread related */

DEFINE_W32API(HANDLE, GetCurrentThread,
	      (void))
{
  debug_message_fn("()\n");
  return (HANDLE)0xfffffffe;
}

DEFINE_W32API(DWORD, GetCurrentThreadId,
	      (void))
{
  debug_message_fn("()\n");
  return getpid();
}

DEFINE_W32API(LCID, GetThreadLocale,
	      (void))
{
  debug_message_fn("()\n");
  return 0;
}

DEFINE_W32API(BOOL, DisableThreadLibraryCalls, (HMODULE module))
{
  debug_message_fn("(module %p)\n", module);
  return TRUE;
}

DEFINE_W32API(HANDLE, CreateThread, (SECURITY_ATTRIBUTES *sa, DWORD stack, LPTHREAD_START_ROUTINE start, LPVOID param, DWORD flags, LPDWORD id))
{
  debug_message_fn("(sa %p, stack %d, start %p, param %p, flags %d, id %p)\n", sa, stack, start, param, flags, id);
  return 0;
}

/* thread local-variable related */

DEFINE_W32API(DWORD, TlsAlloc,
	      (void))
{
  void *p;

  debug_message_fn("()\n");
  if ((p = w32api_mem_alloc(sizeof(void *))) == NULL)
    return HFILE_ERROR;
  return (DWORD)p;
}

DEFINE_W32API(BOOL, TlsFree,
	      (DWORD i))
{
  debug_message_fn("(%p)\n", (void *)i);
  w32api_mem_free((void *)i);
  return TRUE;
}

DEFINE_W32API(LPVOID, TlsGetValue,
	      (DWORD i))
{
  void **p = (void **)i;

  debug_message_fn("(%p)\n", (void *)i);

  return *p;
}

DEFINE_W32API(BOOL, TlsSetValue,
	      (DWORD i, LPVOID value))
{
  void **p = (void **)i;

  debug_message_fn("(%p, %p) ", (void *)i, value);

  *p = value;

  debug_message("OK\n");

  return TRUE;
}

/* critical section */

typedef struct _cs_private {
#ifdef USE_PTHREAD
  pthread_t thread;
  pthread_mutex_t mutex;
#endif
  int is_locked;
} CSPrivate;

DEFINE_W32API(void, InitializeCriticalSection,
	      (CRITICAL_SECTION *cs))
{
  CSPrivate *csp;

  debug_message_fn("()\n");

  if ((csp = calloc(1, sizeof(CSPrivate))) == NULL)
    return;
#ifdef USE_PTHREAD
  pthread_mutex_init(&csp->mutex, NULL);
#endif
  //csp->is_locked = 0;
  *(void **)cs = csp;
  return;
}

DEFINE_W32API(void, EnterCriticalSection,
	      (CRITICAL_SECTION *cs))
{
  CSPrivate *csp = *(CSPrivate **)cs;

  debug_message_fn("()\n");

  if (csp->is_locked)
#ifdef USE_PTHREAD
    if (csp->thread == pthread_self())
#endif
      return;
#ifdef USE_PTHREAD
  pthread_mutex_lock(&csp->mutex);
#endif
  csp->is_locked = 1;
#ifdef USE_PTHREAD
  csp->thread = pthread_self();
#endif

  return;
}

DEFINE_W32API(void, LeaveCriticalSection,
	      (CRITICAL_SECTION *cs))
{
  CSPrivate *csp = *(CSPrivate **)cs;

  debug_message_fn("()\n");

  csp->is_locked = 0;
#ifdef USE_PTHREAD
  pthread_mutex_unlock(&csp->mutex);
#endif

  return;
}

DEFINE_W32API(void, DeleteCriticalSection,
	      (CRITICAL_SECTION *cs))
{
  CSPrivate *csp = *(CSPrivate **)cs;

  debug_message_fn("()\n");

#ifdef USE_PTHREAD
  pthread_mutex_destroy(&csp->mutex);
#endif
  free(csp);

  return;
}

/* Interlocked */
//PVOID WINAPI InterlockedCompareExchange(PVOID*, PVOID, PVOID);
//LONG  WINAPI InterlockedExchange(PLONG, LONG);

DEFINE_W32API(LONG, InterlockedExchangeAdd,
	      (PLONG dest, LONG incr))
{
  LONG ret;
  __asm__ __volatile__("lock; xaddl %0,(%1)"
		       : "=r" (ret)
		       : "r" (dest), "0" (incr)
		       : "memory" );
  return ret;
}

DEFINE_W32API(LONG, InterlockedIncrement,
	      (PLONG dest))
{
    LONG result = InterlockedExchangeAdd(dest, 1) + 1;
    debug_message_fn("(0x%p => %ld) => %ld\n", dest, *dest, result);
    return result;
}

DEFINE_W32API(LONG, InterlockedDecrement,
	      (PLONG dest))
{
    LONG result = InterlockedExchangeAdd(dest, -1) - 1;
    debug_message_fn("(0x%p => %ld) => %ld\n", dest, *dest, result);
    return result;
}

/* exception */

DEFINE_W32API(DWORD, UnhandledExceptionFilter,
	      (PEXCEPTION_POINTERS pp))
{
  debug_message_fn("(%p)\n", pp);
  return 0;
}

DEFINE_W32API(LPTOP_LEVEL_EXCEPTION_FILTER, SetUnhandledExceptionFilter,
	      (LPTOP_LEVEL_EXCEPTION_FILTER filter))
{
  debug_message_fn("(%p)\n", filter);
  return filter;
}

DEFINE_W32API(void, RaiseException,
	      (DWORD code, DWORD flags, DWORD nbargs, const LPDWORD args))
{
  debug_message_fn("(code %d)\n", code);
}

/*
*  this undocumented function is called when an exception
*  handler wants all the frames to be unwound. RtlUnwind
*  calls all exception handlers with the EH_UNWIND or
*  EH_EXIT_UNWIND flags set in the exception record
*
*  This prototype assumes RtlUnwind takes the same
*  parameters as OS/2 2.0 DosUnwindException
*  Disassembling RtlUnwind shows this is true, except for
*  the TargetEIP parameter, which is unused. There is 
*  a fourth parameter, that is used as the eax in the 
*  context.   
*/
DEFINE_W32API(void, RtlUnwind, (PEXCEPTION_FRAME pestframe,
				LPVOID unusedEIP,
				PEXCEPTION_RECORD pexcrec,
				DWORD contextEAX))
{
  debug_message_fn("()\n");
}

/* environment */

DEFINE_W32API(LPSTR, GetCommandLineA,
	      (void))
{
  debug_message_fn("()\n");
  return (LPSTR)"c:\\enfle.exe";
}

/*
 * I usually see __MSVCRT_HEAP_SELECT as name.
 * So, treat it specially.
 */
DEFINE_W32API(DWORD, GetEnvironmentVariableA,
	      (LPCSTR name, LPSTR field,DWORD size))
{
  debug_message_fn("(%s, %p, 0x%x)\n", name, field, size);
  if (strcmp(name, "__MSVCRT_HEAP_SELECT") == 0) {
    strcpy(field, "__GLOBAL_HEAP_SELECTED,1");
  } else {
    if (field) {
      field[0] = '\0';
    }
  }

  return strlen(field);
}

DEFINE_W32API(LPSTR, GetEnvironmentStringsA,
	      (void))
{
  debug_message_fn("()\n");
  return (LPSTR)"Name=Value";
}

DEFINE_W32API(LPWSTR, GetEnvironmentStringsW,
	      (void))
{
  debug_message_fn("()\n");
  return (LPWSTR)0;
}

DEFINE_W32API(BOOL, FreeEnvironmentStringsA,
	      (LPSTR ptr))
{
  debug_message_fn("(%p[%s])\n", ptr, ptr);
  return TRUE;
}

DEFINE_W32API(BOOL, FreeEnvironmentStringsW,
	      (LPWSTR ptr))
{
  debug_message_fn("(%p[%s])\n", ptr, (char *)ptr);
  return TRUE;
}

DEFINE_W32API(VOID, GetStartupInfoA,
	      (LPSTARTUPINFOA info))
{
  debug_message_fn("()\n");
  memset(info, 0, sizeof(*info));
  info->cb = sizeof(*info);
}

/* codepage */

DEFINE_W32API(INT, GetLocaleInfoA,
	      (LCID id, LCTYPE type, LPSTR buf, INT len))
{
  debug_message_fn("(%d, %d, %p, %d)\n", id, type, buf, len);
  if (buf && len)
    buf[0] = '\0';
  return 0;
}

/*
 * CodePages
 * 0000 => 7-bit ASCII
 * 03a4 => Japan (Shift - JIS X-0208)
 * 03b5 => Korea (Shift - KSC 5601)
 * 03b6 => Taiwan (Big5)
 * 04b0 => Unicode
 * 04e2 => Latin-2 (Eastern European)
 * 04e3 => Cyrillic
 * 04e4 => Multilingual
 * 04e5 => Greek
 * 04e6 => Turkish
 * 04e7 => Hebrew
 * 04e8 => Arabic
 */
DEFINE_W32API(UINT, GetACP,
	      (void))
{
  debug_message_fn("()\n");
  return 0;
}

DEFINE_W32API(UINT, GetOEMCP,
	      (void))
{
  debug_message_fn("()\n");
  return 0;
}

DEFINE_W32API(BOOL, GetCPInfo,
	      (UINT cp, LPCPINFO info))
{
  debug_message_fn("(cp %d)\n", cp);
  return TRUE;
}

DEFINE_W32API(BOOL, GetStringTypeA,
	      (LCID id, DWORD type, LPCSTR src, INT count, LPWORD ctype))
{
  debug_message_fn("()\n");
  return TRUE;
}

DEFINE_W32API(BOOL, GetStringTypeW, (DWORD type, LPCWSTR src, INT count, LPWORD ctype))
{
  debug_message_fn("()\n");
  return TRUE;
}

/***********************************************************************
 *              MultiByteToWideChar   (KERNEL32)
 *
 * PARAMS
 *   page [in]    Codepage character set to convert from
 *   flags [in]   Character mapping flags
 *   src [in]     Source string buffer
 *   srclen [in]  Length of source string buffer
 *   dst [in]     Destination buffer
 *   dstlen [in]  Length of destination buffer
 *
 * NOTES
 *   The returned length includes the null terminator character.
 *
 * RETURNS
 *   Success: If dstlen > 0, number of characters written to destination
 *            buffer.  If dstlen == 0, number of characters needed to do
 *            conversion.
 *   Failure: 0. Occurs if not enough space is available.
 *
 * ERRORS
 *   ERROR_INSUFFICIENT_BUFFER
 *   ERROR_INVALID_PARAMETER
 *   ERROR_NO_UNICODE_TRANSLATION
 *
 * INT WINAPI MultiByteToWideChar( UINT page, DWORD flags, LPCSTR src, INT srclen,
 *                                 LPWSTR dst, INT dstlen )
 */
DEFINE_W32API(INT, MultiByteToWideChar,
	      (UINT page, DWORD flags, LPCSTR src, INT srclen,
	       LPWSTR dest, INT destlen))
{
  debug_message_fn("()\n");

  if (dest && destlen > 0)
    *dest = 0;

  return 0;
}

/***********************************************************************
 *              WideCharToMultiByte   (KERNEL32)
 *
 * PARAMS
 *   page [in]    Codepage character set to convert to
 *   flags [in]   Character mapping flags
 *   src [in]     Source string buffer
 *   srclen [in]  Length of source string buffer
 *   dst [in]     Destination buffer
 *   dstlen [in]  Length of destination buffer
 *   defchar [in] Default character to use for conversion if no exact
 *                  conversion can be made
 *   used [out]   Set if default character was used in the conversion
 *
 * NOTES
 *   The returned length includes the null terminator character.
 *
 * RETURNS
 *   Success: If dstlen > 0, number of characters written to destination
 *            buffer.  If dstlen == 0, number of characters needed to do
 *            conversion.
 *   Failure: 0. Occurs if not enough space is available.
 *
 * ERRORS
 *   ERROR_INSUFFICIENT_BUFFER
 *   ERROR_INVALID_PARAMETER
 *
 * INT WINAPI WideCharToMultiByte( UINT page, DWORD flags, LPCWSTR src, INT srclen,
 *                                 LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
 */
DEFINE_W32API(INT, WideCharToMultiByte,
	      (UINT page, DWORD flags, LPCWSTR src, INT srclen,
	       LPSTR dest, INT destlen, LPCSTR defchar, BOOL *used))
{
  debug_message_fn("()\n");

  if (dest && destlen > 0)
    *dest = 0;

  return 0;
}

/*
LCID lcid: Locale identifier created with MAKELCID;
           LOCALE_SYSTEM_DEFAULT and LOCALE_USER_DEFAULT are
           predefined values.
DWORD mapflags: Flags.
LPCSTR srcstr: Source buffer.
INT srclen: Source length.
LPSTR dststr: Destination buffer.
INT dstlen: Destination buffer length.

Convert a string, or generate a sort key from it.  If (mapflags &
LCMAP_SORTKEY), the function will generate a sort key for the source
string. Else, it will convert it accordingly to the flags
LCMAP_UPPERCASE, LCMAP_LOWERCASE,...
*/
DEFINE_W32API(INT, LCMapStringA,
	      (LCID lcid, DWORD mapflags, LPCSTR srcstr, INT srclen, LPSTR dststr, INT dstlen))
{
  debug_message_fn("(%d, %d, %p, %d, %p, %d)\n", lcid, mapflags, srcstr, srclen, dststr, dstlen);
  return 0;
}

DEFINE_W32API(BOOL, IsDBCSLeadByte,
	      (BYTE c))
{
  return FALSE;
}

/* date and time */

DEFINE_W32API(BOOL, EnumCalendarInfoA,
	      (CALINFO_ENUMPROCA proc, LCID id, CALID calid, CALTYPE caltype))
{
  debug_message_fn("()\n");
  return FALSE;
}

DEFINE_W32API(VOID, GetLocalTime,
	      (LPSYSTEMTIME t))
{
  debug_message_fn("(%p)\n", t);
}

/* miscellaneous */

DEFINE_W32API(DWORD, GetLastError,
	      (void))
{
  debug_message_fn("(): 0\n");
  return 0;
}

DEFINE_W32API(void, SetLastError, (DWORD e))
{
  debug_message_fn("(%d)\n", e);
}

DEFINE_W32API(LONG, GetVersion,
	      (void))
{
  debug_message_fn("()\n");

  return 0xc0000a04; /* Windows98 */
}

DEFINE_W32API(BOOL, GetVersionExA,
	      (OSVERSIONINFOA *v))
{
  debug_message_fn("()\n");

  v->dwMajorVersion = 4;
  v->dwMinorVersion = 10;
  v->dwBuildNumber = 0x40a07ce;
  v->dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
  strcpy(v->szCSDVersion, "Win98");

  return TRUE;
}

/* unimplemened */

DEFINE_W32API(void, unknown_symbol,
	      (void))
{
  show_message("unknown symbol in kernel32 called\n");
}

/* export */

Symbol_info *
kernel32_get_export_symbols(void)
{
  module_register("kernel32.dll", symbol_infos);
  return symbol_infos;
}
