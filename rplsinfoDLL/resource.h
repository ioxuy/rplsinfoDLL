#pragma once

// Application Data
#ifdef DEBUG
#define VER_DEBUG   VS_FF_DEBUG
#define VER_PRIVATE VS_FF_PRIVATEBUILD
#else
#define VER_DEBUG   0
#define VER_PRIVATE 0
#endif

#define VER_STR_COMMENTS         L""
#define VER_STR_COMPANYNAME      L""
#define VER_STR_FILEDESCRIPTION  L"rplsinfo DLL Version"
#define VER_FILEVERSION          1,3
#define VER_STR_FILEVERSION      L"1.3"
#define VER_STR_INTERNALNAME     L"rplsinfo"
#define VER_STR_ORIGINALFILENAME L"rplsinfoDLL.dll"
#define VER_STR_LEGALCOPYRIGHT   L"Modified by M.Nakamura"
#define VER_STR_PRODUCTNAME      L"rplsinfoDLL"
#define VER_PRODUCTVERSION       1,0,8,4
#define VER_STR_PRODUCTVERSION   L"1.0.8.4"

// Resource ID
#define IDR_VERSION_INFO 1000
