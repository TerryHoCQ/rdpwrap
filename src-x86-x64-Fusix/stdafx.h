// stdafx.h: Precompiled header file  
#pragma once  
#include "targetver.h"  

#define WIN32_LEAN_AND_MEAN  
#define _CRT_SECURE_NO_WARNINGS  

// Include necessary Windows headers  
#include <windows.h>  
#include <winbase.h>  
#include <tlhelp32.h>  // For THREADENTRY32

// Define types and other necessary items here  
typedef VOID (WINAPI* SERVICEMAIN)(DWORD, LPTSTR*); 
typedef VOID (WINAPI* SVCHOSTPUSHSERVICEGLOBALS)(VOID*); 
typedef HRESULT (WINAPI* SLGETWINDOWSINFORMATIONDWORD)(PCWSTR, DWORD*);
