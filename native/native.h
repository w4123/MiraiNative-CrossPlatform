#pragma once
#ifndef _WIN32
#define __stdcall
#endif 

#if (defined _WIN32 && !defined _WIN64)
#ifdef _MSC_VER
#define CQAPI(ReturnType, Name, Size) __pragma(comment(linker, "/EXPORT:" #Name "=_" #Name "@" #Size)) extern "C" __declspec(dllexport) ReturnType __stdcall Name
#else
#define CQAPI(ReturnType, Name, Size) extern "C" __attribute__((dllexport)) ReturnType __attribute__((__stdcall__)) Name
#endif
#elif defined _WIN64
#ifdef _MSC_VER
#define CQAPI(ReturnType, Name, Size) extern "C" __declspec(dllexport) ReturnType Name
#else
#define CQAPI(ReturnType, Name, Size) extern "C" __attribute__((dllexport)) ReturnType Name
#endif
#else
#define CQAPI(ReturnType, Name, Size) extern "C" __attribute__((visibility ("default"))) ReturnType Name
#endif

typedef int32_t (__stdcall* IntMethod)();
typedef const char* (__stdcall* StringMethod)();
typedef int32_t (__stdcall* FuncInitialize)(int32_t);

typedef int32_t (__stdcall* EvPriMsg)(int32_t, int32_t, int64_t, const char*, int32_t);
typedef int32_t (__stdcall* EvGroupMsg)(int32_t, int32_t, int64_t, int64_t, const char*, const char*, int32_t);
typedef int32_t (__stdcall* EvGroupAdmin)(int32_t, int32_t, int64_t, int64_t);
typedef int32_t (__stdcall* EvGroupMember)(int32_t, int32_t, int64_t, int64_t, int64_t);
typedef int32_t (__stdcall* EvGroupBan)(int32_t, int32_t, int64_t, int64_t, int64_t, int64_t);
typedef int32_t (__stdcall* EvRequestAddGroup)(int32_t, int32_t, int64_t, int64_t, const char*, const char*);
typedef int32_t (__stdcall* EvRequestAddFriend)(int32_t, int32_t, int64_t, const char*, const char*);
typedef int32_t (__stdcall* EvFriendAdd)(int32_t, int32_t, int64_t);