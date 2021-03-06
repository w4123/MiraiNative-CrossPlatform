﻿#include <jni.h>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <queue>
#include <cstring>
#include <fstream>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#define BOOL int32_t
#define FALSE 0
#define TRUE 1
#endif
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include "org_itxtech_mirainative_Bridge.h"
#include "native.h"

using namespace std;

struct native_plugin
{
	int id;
	const char* file;
#ifdef _WIN32
	HMODULE dll;
#else
	void* dll;
#endif
	bool enabled;

	native_plugin()
	{
		id = -1;
		file = "";
		dll = nullptr;
		enabled = false;
	};

	native_plugin(int i, char* f)
	{
		id = i;
		file = f;
		dll = nullptr;
		enabled = true;
	}
};

priority_queue<pair<time_t, const char*>> mem_queue;
mutex mem_mutex;
bool running = true;

thread mem_thread([]
{
	while (running)
	{
		{
			unique_lock<mutex> lock(mem_mutex);
			while (!mem_queue.empty() && time(nullptr) - mem_queue.top().first > 300)
			{
				free((void*)mem_queue.top().second);
				mem_queue.pop();
			}
		}
		this_thread::sleep_for(500ms);
	}
});

const char* delay_mem_free(const char* str)
{
	unique_lock<mutex> lock(mem_mutex);
	mem_queue.push({time(nullptr), str});
	return str;
}

// Global var

map<int, native_plugin> plugins;

// Strings

// Memory returned from this function need to be freed using free()
const char* ByteArrayToChars(JNIEnv* env, jbyteArray arr)
{
	jsize len = env->GetArrayLength(arr);
	char* buffer = (char*)malloc(len + 1);
	jbyte* elements = env->GetByteArrayElements(arr, nullptr);
	memcpy(buffer, elements, len);
	buffer[len] = '\0';
	env->ReleaseByteArrayElements(arr, elements, JNI_ABORT);
	return buffer;
}

string ByteArrayToString(JNIEnv* env, jbyteArray arr)
{
	const auto* buf = ByteArrayToChars(env, arr);
	string s(buf);
	free((void*)buf);
	return s;
}

jbyteArray CharsToByteArray(JNIEnv* env, const char* str)
{
	if (str == nullptr)
	{
		str = "\0";
	}
	auto len = strlen(str);
	auto arr = env->NewByteArray(len);
	env->SetByteArrayRegion(arr, 0, len, reinterpret_cast<const jbyte*>(str));
	return arr;
}

#ifdef _WIN32
FARPROC GetMethod(JNIEnv* env, jint id, jbyteArray method)
#else
void* GetMethod(JNIEnv* env, jint id, jbyteArray method)
#endif
{
#ifdef _WIN32
	return GetProcAddress(plugins[id].dll, ByteArrayToString(env, method).c_str());
#else
	return dlsym(plugins[id].dll, ByteArrayToString(env, method).c_str());
#endif
}

// Load

JavaVM* jvm = nullptr;
jclass bclz = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
	{
		return -1;
	}
	jvm = vm;
	auto b = env->FindClass("org/itxtech/mirainative/Bridge");
	bclz = jclass(env->NewGlobalRef(b));

	return JNI_VERSION_1_6;
}

pair<JNIEnv*, bool> attach_java()
{
	JNIEnv* env = nullptr;
	if (jvm)
	{
		const int stat = jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
		if (stat == JNI_EDETACHED)
		{
			JavaVMAttachArgs args = {JNI_VERSION_1_6, nullptr, nullptr};
#ifdef __ANDROID__
			jvm->AttachCurrentThread(&env, &args);
#else
			jvm->AttachCurrentThread(reinterpret_cast<void**>(&env), &args);
#endif
			return { env, true };
		}
	}
	return { env, false };
}

void detach_java()
{
	jvm->DetachCurrentThread();
}


// Utilities

#ifndef _WIN32
std::vector<void*> vec_handles;
#endif

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_exportGlobalSymbol(JNIEnv* env, jclass clz, jbyteArray file)
{
#ifndef _WIN32
	vec_handles.push_back(dlopen(ByteArrayToString(env, file).c_str(), RTLD_LAZY | RTLD_GLOBAL));
#endif
#ifdef __ANDROID__
	string file_path = ByteArrayToString(env, file);
	file_path = file_path.substr(0, file_path.rfind('/'));
	string ld_path;
	void* dl = dlopen("libdl.so", RTLD_NOW | RTLD_GLOBAL);
	void* dla = dlopen("libdl_android.so", RTLD_NOW | RTLD_GLOBAL);
	void* sym1 = dlsym(RTLD_DEFAULT, "android_get_LD_LIBRARY_PATH");
	if (!sym1 && dl) sym1 = dlsym(dl, "android_get_LD_LIBRARY_PATH");
	if (!sym1 && dla) sym1 = dlsym(dla, "android_get_LD_LIBRARY_PATH");
	if (sym1 != NULL) 						
	{
		char buffer[300];
		typedef void (*Fn)(char*, size_t);
		Fn android_get_LD_LIBRARY_PATH = reinterpret_cast<Fn>(sym1);
		(*android_get_LD_LIBRARY_PATH)(buffer, 300);
		ld_path = buffer;
	}
	else 
	{
		__android_log_print(ANDROID_LOG_WARN, "MiraiNative", "android_get_LD_LIBRARY_PATH failed");
	}
	void* sym2 = dlsym(RTLD_DEFAULT, "android_update_LD_LIBRARY_PATH");
	if (!sym2 && dl) sym2 = dlsym(dl, "android_update_LD_LIBRARY_PATH");
	if (!sym2 && dla) sym2 = dlsym(dla, "android_update_LD_LIBRARY_PATH");
	if (sym2 != NULL) 
	{
		string tmp;
		for (const auto c : ld_path)
		{
			if (c == ':')
			{
				if (tmp == file_path)
				{
					if (dl) dlclose(dl);
					if (dla) dlclose(dla);
					return 0;
				}
				tmp = "";
			}
			else tmp += c;
		}
		if (tmp == file_path)
		{
			if (dl) dlclose(dl);
			if (dla) dlclose(dla);
			return 0;
		}
		
		if(!ld_path.empty() && ld_path[ld_path.length() - 1] != ':')
		{
			ld_path += ":";
		}
		
		ld_path += file_path;
		
		typedef void (*Fn)(const char*);
		Fn android_update_LD_LIBRARY_PATH = reinterpret_cast<Fn>(sym2);
		(*android_update_LD_LIBRARY_PATH)(ld_path.c_str());
	}
	else
	{
		__android_log_print(ANDROID_LOG_WARN, "MiraiNative", "android_update_LD_LIBRARY_PATH failed");
	}
	if (dl) dlclose(dl);
	if (dla) dlclose(dla);
#endif
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_shutdown(JNIEnv* env, jclass clz)
{
	env->DeleteGlobalRef(bclz);
	running = false;
	mem_thread.join();
#ifndef _WIN32
	for (const auto& handle : vec_handles)
	{
		dlclose(handle);
	}
	vec_handles.clear();
#endif
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_setCurrentDirectory(JNIEnv* env, jclass clz, jbyteArray dir)
{
#ifdef _WIN32
	SetCurrentDirectoryA(ByteArrayToString(env, dir).c_str());
#else
	chdir(ByteArrayToString(env, dir).c_str());
#endif
	return 0;
}

// Plugin

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_loadNativePlugin(
	JNIEnv* env, jclass clz, jbyteArray file, jint id)
{
	native_plugin plugin = {id, const_cast<char*>(ByteArrayToChars(env, file))};
#ifdef _WIN32
	const auto dll = LoadLibraryA(plugin.file);
#else
	const auto dll = dlopen(plugin.file, RTLD_LAZY);
#endif

	if (dll != nullptr)
	{
		plugin.dll = dll;
		plugins[id] = plugin;

#ifdef _WIN32
		const auto init = FuncInitialize(GetProcAddress(dll, "Initialize"));
#else
		const auto init = FuncInitialize(dlsym(dll, "Initialize"));
#endif
		if (init)
		{
			init(plugin.id);
		}

		return 0;
	}
#ifdef _WIN32
	return GetLastError();
#else
	return dlerror() == nullptr ? 0 : -1;
#endif
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_freeNativePlugin(
	JNIEnv* env, jclass clz, jint id)
{
#ifdef _WIN32
	auto r = FreeLibrary(plugins[id].dll);
#else
	auto r = dlclose(plugins[id].dll);
#endif
	free((void*)plugins[id].file);
	if (r != FALSE)
	{
		return 0;
	}
#ifdef _WIN32
	return GetLastError();
#else
	return dlerror() == nullptr ? 0 : -1;
#endif
}

JNIEXPORT void JNICALL Java_org_itxtech_mirainative_Bridge_processMessage(JNIEnv* env, jclass clz)
{
#ifdef _WIN32
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_callIntMethod(
	JNIEnv* env, jclass clz, jint id, jbyteArray method)
{
	const auto m = IntMethod(GetMethod(env, id, method));
	if (m)
	{
		return m();
	}
	return 0;
}

JNIEXPORT jbyteArray JNICALL Java_org_itxtech_mirainative_Bridge_callStringMethod(
	JNIEnv* env, jclass clz, jint id, jbyteArray method)
{
	const char* rtn = "";
	const auto m = StringMethod(GetMethod(env, id, method));
	if (m)
	{
		rtn = m();
	}
	return CharsToByteArray(env, rtn);
}

// Event

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvPrivateMessage(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint msg_id, jlong acct, jbyteArray msg, jint font)
{
	const auto m = EvPriMsg(GetMethod(env, id, method));
	if (m)
	{
		auto result = m(type, msg_id, acct, ByteArrayToString(env, msg).c_str(), font);
		return result;
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvGroupMessage(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint msg_id, jlong grp,
	jlong acct, jbyteArray anon, jbyteArray msg, jint font)
{
	const auto m = EvGroupMsg(GetMethod(env, id, method));
	if (m)
	{
		auto result = m(type, msg_id, grp, acct, ByteArrayToString(env, anon).c_str(),
		                ByteArrayToString(env, msg).c_str(), font);
		return result;
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvGroupAdmin(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint time, jlong grp, jlong acct)
{
	const auto m = EvGroupAdmin(GetMethod(env, id, method));
	if (m)
	{
		return m(type, time, grp, acct);
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvGroupMember(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint time, jlong grp, jlong acct, jlong mbr)
{
	const auto m = EvGroupMember(GetMethod(env, id, method));
	if (m)
	{
		return m(type, time, grp, acct, mbr);
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvGroupBan(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint time, jlong grp,
	jlong acct, jlong mbr, jlong dur)
{
	const auto m = EvGroupBan(GetMethod(env, id, method));
	if (m)
	{
		return m(type, time, grp, acct, mbr, dur);
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvRequestAddGroup(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint time,
	jlong grp, jlong acct, jbyteArray msg, jbyteArray flag)
{
	const auto m = EvRequestAddGroup(GetMethod(env, id, method));
	if (m)
	{
		auto result = m(type, time, grp, acct, ByteArrayToString(env, msg).c_str(),
		                ByteArrayToString(env, flag).c_str());
		return result;
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvRequestAddFriend(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint time,
	jlong acct, jbyteArray msg, jbyteArray flag)
{
	const auto m = EvRequestAddFriend(GetMethod(env, id, method));
	if (m)
	{
		auto result = m(type, time, acct, ByteArrayToString(env, msg).c_str(), ByteArrayToString(env, flag).c_str());
		return result;
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_org_itxtech_mirainative_Bridge_pEvFriendAdd(
	JNIEnv* env, jclass clz, jint id, jbyteArray method, jint type, jint time, jlong acct)
{
	const auto m = EvFriendAdd(GetMethod(env, id, method));
	if (m)
	{
		return m(type, time, acct);
	}
	return 0;
}

// Mirai Unique

CQAPI(int32_t, isMiraiNative, 0)()
{
	return 1;
}

CQAPI(int32_t, mQuoteMessage, 12)(int32_t plugin_id, int32_t msg_id, const char* msg)
{
	auto [env, need_detach] = attach_java();
	auto m = CharsToByteArray(env, msg);
	auto method = env->GetStaticMethodID(bclz, "quoteMessage", "(II[B)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, msg_id, m);
	env->DeleteLocalRef(m);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, mForwardMessage, 24)(int32_t plugin_id, int32_t type, int64_t id, const char* strategy, const char* msg)
{
	auto [env, need_detach] = attach_java();
	auto s = CharsToByteArray(env, strategy);
	auto m = CharsToByteArray(env, msg);
	auto method = env->GetStaticMethodID(bclz, "forwardMessage", "(IIJ[B[B)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, type, id, s, m);
	env->DeleteLocalRef(s);
	env->DeleteLocalRef(m);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, mSetGroupKick, 28)(int32_t plugin_id, int64_t group, int64_t member, BOOL reject, const char* msg)
{
	auto [env, need_detach] = attach_java();
	auto m = CharsToByteArray(env, msg);
	auto method = env->GetStaticMethodID(bclz, "setGroupKick", "(IJJZ[B)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, member, reject != FALSE, msg);
	env->DeleteLocalRef(m);
	if (need_detach) detach_java();
	return result;
}

CQAPI(const char*, mGetGroupEntranceAnnouncement, 12)(int32_t plugin_id, int64_t group)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getGroupEntranceAnnouncement", "(IJ)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, group));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(int32_t, mSetGroupEntranceAnnouncement, 16)(int32_t plugin_id, int64_t group, const char* a)
{
	auto [env, need_detach] = attach_java();
	auto an = CharsToByteArray(env, a);
	auto method = env->GetStaticMethodID(bclz, "setGroupEntranceAnnouncement", "(IJ[B)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, an);
	env->DeleteLocalRef(an);
	if (need_detach) detach_java();
	return result;
}

// CQ APIs

CQAPI(int32_t, CQ_addLog, 16)(int32_t plugin_id, int32_t priority, const char* type, const char* content)
{
	auto [env, need_detach] = attach_java();
	auto t = CharsToByteArray(env, type);
	auto c = CharsToByteArray(env, content);
	auto method = env->GetStaticMethodID(bclz, "addLog", "(II[B[B)V");
	env->CallStaticVoidMethod(bclz, method, plugin_id, priority, t, c);
	env->DeleteLocalRef(t);
	env->DeleteLocalRef(c);
	if (need_detach) detach_java();
	return 0;
}

CQAPI(int32_t, CQ_canSendImage, 4)(int32_t)
{
	return 1;
}

CQAPI(int32_t, CQ_canSendRecord, 4)(int32_t)
{
	return 1;
}

int32_t sendMsg(int32_t plugin_id, int64_t acc, const char* msg, const char* m)
{
	auto [env, need_detach] = attach_java();
	auto jstr = CharsToByteArray(env, msg);
	auto method = env->GetStaticMethodID(bclz, m, "(IJ[B)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, acc, jstr);
	env->DeleteLocalRef(jstr);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_sendPrivateMsg, 16)(int32_t plugin_id, int64_t account, const char* msg)
{
	return sendMsg(plugin_id, account, msg, "sendPrivateMessage");
}

CQAPI(int32_t, CQ_sendGroupMsg, 16)(int32_t plugin_id, int64_t group, const char* msg)
{
	return sendMsg(plugin_id, group, msg, "sendGroupMessage");
}

CQAPI(int32_t, CQ_setFatal, 8)(int32_t plugin_id, const char* info)
{
	CQ_addLog(plugin_id, 22, "", info);
	return 0;
}

CQAPI(const char*, CQ_getAppDirectory, 4)(int32_t plugin_id)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getPluginDataDir", "(I)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(int64_t, CQ_getLoginQQ, 4)(int32_t plugin_id)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getLoginQQ", "(I)J");
	auto result = env->CallStaticLongMethod(bclz, method, plugin_id);
	if (need_detach) detach_java();
	return result;
}

CQAPI(const char*, CQ_getLoginNick, 4)(int32_t plugin_id)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getLoginNick", "(I)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(int32_t, CQ_setGroupAnonymous, 16)(int32_t plugin_id, int64_t group, BOOL enable)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupAnonymous", "(IJZ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, enable != FALSE);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupBan, 28)(int32_t plugin_id, int64_t group, int64_t member, int64_t duration)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupBan", "(IJJJ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, member, duration);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupCard, 24)(int32_t plugin_id, int64_t group, int64_t member, const char* card)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupCard", "(IJJ[B)I");
	auto jstr = CharsToByteArray(env, card);
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, member, jstr);
	env->DeleteLocalRef(jstr);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupKick, 24)(int32_t plugin_id, int64_t group, int64_t member, BOOL reject)
{
	return mSetGroupKick(plugin_id, group, member, reject, "");
}

CQAPI(int32_t, CQ_setGroupLeave, 16)(int32_t plugin_id, int64_t group, BOOL dismiss)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupLeave", "(IJZ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, dismiss != FALSE);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupSpecialTitle, 32)(int32_t plugin_id, int64_t group, int64_t member,
                                            const char* title, int64_t duration)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupSpecialTitle", "(IJJ[BJ)I");
	auto jstr = CharsToByteArray(env, title);
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, member, jstr, duration);
	env->DeleteLocalRef(jstr);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupWholeBan, 16)(int32_t plugin_id, int64_t group, BOOL enable)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupWholeBan", "(IJZ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, enable != FALSE);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_deleteMsg, 12)(int32_t plugin_id, int64_t msg_id)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "recallMsg", "(IJ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, msg_id);
	if (need_detach) detach_java();
	return result;
}

CQAPI(const char*, CQ_getFriendList, 8)(int32_t plugin_id, BOOL reserved)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getFriendList", "(IZ)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, reserved != FALSE));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getGroupInfo, 16)(int32_t plugin_id, int64_t group, BOOL cache)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getGroupInfo", "(IJZ)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, group, cache != FALSE));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getGroupList, 4)(int32_t plugin_id)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getGroupList", "(I)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getGroupMemberInfoV2, 24)(int32_t plugin_id, int64_t group, int64_t account, BOOL cache)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getGroupMemberInfo", "(IJJZ)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, group, account, cache != FALSE));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getGroupMemberList, 12)(int32_t plugin_id, int64_t group)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getGroupMemberList", "(IJ)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, group));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getCookiesV2, 8)(int32_t plugin_id, const char* domain)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getCookies", "(I[B)[B");
	auto jstr = CharsToByteArray(env, domain);
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, jstr));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	env->DeleteLocalRef(jstr);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getCsrfToken, 4)(int32_t plugin_id)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getCsrfToken", "(I)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getImage, 8)(int32_t plugin_id, const char* image)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getImage", "(I[B)[B");
	auto jstr = CharsToByteArray(env, image);
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, jstr));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	env->DeleteLocalRef(jstr);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getRecordV2, 12)(int32_t plugin_id, const char* file, const char* format)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getRecord", "(I[B[B)[B");
	auto f = CharsToByteArray(env, file);
	auto fmt = CharsToByteArray(env, format);
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, f, fmt));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(f);
	env->DeleteLocalRef(fmt);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(const char*, CQ_getStrangerInfo, 16)(int32_t plugin_id, int64_t account, BOOL cache)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "getStrangerInfo", "(IJZ)[B");
	auto result = jbyteArray(env->CallStaticObjectMethod(bclz, method, plugin_id, account, cache != FALSE));
	auto r = ByteArrayToChars(env, result);
	env->DeleteLocalRef(result);
	if (need_detach) detach_java();
	return delay_mem_free(r);
}

CQAPI(int32_t, CQ_sendDiscussMsg, 16)(int32_t plugin_id, int64_t group, const char* msg)
{
	return sendMsg(plugin_id, group, msg, "sendDiscussMessage");
}

CQAPI(int32_t, CQ_sendLikeV2, 16)(int32_t plugin_id, int64_t account, int32_t times)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "sendLike", "(IJI)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, account, times);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setDiscussLeave, 12)(int32_t plugin_id, int64_t group)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setDiscussLeave", "(IJ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setFriendAddRequest, 16)(int32_t plugin_id, const char* id, int32_t type, const char* remark)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setFriendAddRequest", "(I[BI[B)I");
	auto i = CharsToByteArray(env, id);
	auto r = CharsToByteArray(env, remark);
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, i, type, r);
	env->DeleteLocalRef(i);
	env->DeleteLocalRef(r);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupAddRequestV2, 20)(int32_t plugin_id, const char* id, int32_t req_type, int32_t fb_type,
                                            const char* reason)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupAddRequest", "(I[BII[B)I");
	auto i = CharsToByteArray(env, id);
	auto r = CharsToByteArray(env, reason);
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, i, req_type, fb_type, r);
	env->DeleteLocalRef(i);
	env->DeleteLocalRef(r);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupAdmin, 24)(int32_t plugin_id, int64_t group, int64_t account, BOOL admin)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupAdmin", "(IJJZ)I");
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, account, admin != FALSE);
	if (need_detach) detach_java();
	return result;
}

CQAPI(int32_t, CQ_setGroupAnonymousBan, 24)(int32_t plugin_id, int64_t group, const char* id, int64_t duration)
{
	auto [env, need_detach] = attach_java();
	auto method = env->GetStaticMethodID(bclz, "setGroupAnonymousBan", "(IJ[BJ)I");
	auto i = CharsToByteArray(env, id);
	auto result = env->CallStaticIntMethod(bclz, method, plugin_id, group, i, duration);
	env->DeleteLocalRef(i);
	if (need_detach) detach_java();
	return result;
}

// Legacy

CQAPI(const char*, CQ_getCookies, 4)(int32_t plugin_id)
{
	return CQ_getCookiesV2(plugin_id, "");
}

CQAPI(int32_t, CQ_setGroupAddRequest, 16)(int32_t plugin_id, const char* id, int32_t req_type, int32_t fb_type)
{
	return CQ_setGroupAddRequestV2(plugin_id, id, req_type, fb_type, "");
}

CQAPI(int32_t, CQ_sendLike, 12)(int32_t plugin_id, int64_t account)
{
	return CQ_sendLikeV2(plugin_id, account, 1);
}

CQAPI(int32_t, CQ_setFunctionMark, 8)(int32_t plugin_id, const char* name)
{
	return 0;
}

CQAPI(const char*, CQ_getRecord, 12)(int32_t plugin_id, const char* file, const char* format)
{
	return CQ_getRecordV2(plugin_id, file, format);
}
