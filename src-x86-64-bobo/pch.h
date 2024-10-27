// pch.h: 标准系统包含文件或项目特定包含文件的包含文件，这些文件经常使用但不经常修改

#pragma once

#include "framework.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除不常用的组件
#define _CRT_SECURE_NO_WARNINGS

// Windows 头文件:
#include <windows.h>
#include <TlHelp32.h>

// TODO: 在此处设置对项目所需的其他头文件的引用

typedef VOID(WINAPI* SERVICEMAIN)(DWORD, LPTSTR*);
typedef VOID(WINAPI* SVCHOSTPUSHSERVICEGLOBALS)(VOID*);
typedef HRESULT(WINAPI* SLGETWINDOWSINFORMATIONDWORD)(PCWSTR, DWORD*);
#pragma once
