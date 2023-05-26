#include <Corepch.h>
// #include "Windows/WindowsAPI.h"
// #include "Windows/WindowsCommon.h"
//
// namespace Zn::Windows
//{
//	//#ifdef __clang__
//	HANDLE WINAPI OpenThread(DWORD desired_access, BOOL inherit_handle, DWORD thread_id)
//	{
//		return ::OpenThread(desired_access, inherit_handle, thread_id);
//	}
//	HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES security_attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_routine, LPVOID
//parameter, DWORD creation_flags, LPDWORD thread_id)
//	{
//		return ::CreateThread(security_attributes, stack_size, start_routine, parameter, creation_flags, thread_id);
//	}
//	void WINAPI SetThreadPriority(HANDLE thread_handle, int thread_priority)
//	{
//		::SetThreadPriority(thread_handle, thread_priority);
//	}
//	void WINAPI TerminateThread(HANDLE thread_handle, DWORD exit_code)
//	{
//		::TerminateThread(thread_handle, exit_code);
//	}
//	DWORD WINAPI GetCurrentThreadId()
//	{
//		return ::GetCurrentThreadId();
//	}
//	DWORD WINAPI WaitForSingleObject(HANDLE handle, DWORD ms)
//	{
//		return ::WaitForSingleObject(handle, ms);
//	}
//	void WINAPI Sleep(DWORD ms)
//	{
//		::Sleep(ms);
//	}
//	void WINAPI CloseHandle(HANDLE handle)
//	{
//		::CloseHandle(handle);
//	}
//	//#endif
// }
