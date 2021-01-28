#pragma once
#ifndef _WIN32
#define __stdcall
#endif 

#define CQAPI(ReturnType, Name, Size) extern "C" ReturnType __stdcall Name

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