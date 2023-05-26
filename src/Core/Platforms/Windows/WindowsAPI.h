// #pragma once
// #include "Types.h"
//
// #ifdef __clang__
//	#define WINDOWS_API
// #else
//	#define WINDOWS_API extern "C" __declspec(dllimport)
// #endif
//
// #pragma push_macro("TRUE")
// #pragma push_macro("FALSE")
// #undef TRUE
// #undef FALSE
//
// struct _RTL_CRITICAL_SECTION;
//
// #ifndef WINAPI
// #define WINAPI __stdcall
// #endif
//
// namespace Zn::Windows
//{
//	typedef int32 BOOL;
//	typedef unsigned long DWORD;
//	typedef DWORD* LPDWORD;
//	typedef long LONG;
//	typedef long* LPLONG;
//	typedef int64 LONGLONG;
//	typedef LONGLONG* LPLONGLONG;
//	typedef void* LPVOID;
//	typedef const void* LPCVOID;
//	typedef const wchar_t* LPCTSTR;
//	typedef size_t SIZE_T;
//
//	typedef void* HANDLE;
//
//	typedef _RTL_CRITICAL_SECTION* LPCRITICAL_SECTION;
//
//	static constexpr BOOL TRUE = 1;
//	static constexpr BOOL FALSE = 0;
//
//
//	typedef struct _SECURITY_ATTRIBUTES {
//		DWORD Length;
//		LPVOID SecurityDescriptor;
//		BOOL InheritHandle;
//	} SECURITY_ATTRIBUTES, * PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
//
//	typedef DWORD(WINAPI* PTHREAD_START_ROUTINE)(
//		LPVOID ThreadParameter
//		);
//
//	typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;
//
//	WINDOWS_API HANDLE WINAPI OpenThread(DWORD desired_access, BOOL inherit_handle, DWORD thread_id);
//	WINDOWS_API HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_routine, LPVOID parameter,
//DWORD creation_flags, LPDWORD thread_id); 	WINDOWS_API void WINAPI SetThreadPriority(HANDLE thread_handle, int thread_priority); 	WINDOWS_API void WINAPI
//TerminateThread(HANDLE thread_handle, DWORD exit_code); 	WINDOWS_API DWORD WINAPI GetCurrentThreadId(); 	WINDOWS_API DWORD WINAPI WaitForSingleObject(HANDLE
//handle, DWORD ms); 	WINDOWS_API void WINAPI Sleep(DWORD ms); 	WINDOWS_API void WINAPI CloseHandle(HANDLE handle);
//
// }
//
// #pragma pop_macro("FALSE")
// #pragma pop_macro("TRUE")
