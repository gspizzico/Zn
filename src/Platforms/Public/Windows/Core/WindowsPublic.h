#pragma once

// Typedeffing windows types to avoid including windows.h in header files.

using HANDLE = void*;
using DWORD  = unsigned long;
using LPVOID = void*;
typedef DWORD(__stdcall* PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
using LPTHREAD_START_ROUTINE = PTHREAD_START_ROUTINE;
