/*
 * GlobalHook.c
 *
 * 이번에는 무슨 문제인가...!
 *
 *  Created on: 2016. 7. 17.
 *      Author: 지수
 */

#include <stdio.h>
#include <windows.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "redlaboratory_globalinputmanager_GlobalHook.h"

#define UNC_PIPE_NAME "\\\\.\\pipe\\RLHookPipe"// Universal Name Convention
#define NATIVE_EVENT_CLASS "redlaboratory/globalinputmanager/GlobalHook"

//#define INFO_LOGGING

#ifdef INFO_LOGGING
	#define info(format, ...) _info(format, ##__VA_ARGS__)

	void _info(char* string, ...) {
		char* file = "D:/output.txt";

		va_list formats;
		va_start(formats, string);

		FILE *f = fopen(file, "a");
		vfprintf(f, string, formats);
		vprintf(string, formats);
		fclose(f);

		va_end(formats);
	}

	char* getModuleName() {
		static char path[MAX_PATH];
		GetModuleFileNameA(NULL, path, MAX_PATH);

		return path;
	}
#else
	#define info(format, ...)
#endif

#define MT_ACCEPT 0
#define MT_KEYBOARD 1
#define MT_MOUSE 2
#define MT_TERMINATE 3

#define MOUSE_LEFT 0
#define MOUSE_RIGHT 1
#define MOUSE_MIDDLE 2
#define MOUSE_XBUTTON1 3
#define MOUSE_XBUTTON2 4

#define MOUSE_DOWN 0
#define MOUSE_UP 1
#define MOUSE_DB_CLICK 2
#define MOUSE_MOVE 3
#define MOUSE_WHEEL_SCROLL 4

#pragma pack(push, 1)
typedef struct _Message {
	int messageType;// 0 = ignore this message, 1 = evaluate this message, 2 = terminate the thread

	int vk;
	BOOL released;
	BOOL extendedKey;
	BOOL altDowned;
	BOOL ctrlDowned;
	BOOL shiftDowned;

	int mouseEventType;
	int mouseButton;
	LONG x;
	LONG y;
	short delta;
} Message;
#pragma pack(pop)

// server

JavaVM* pJavaVM = NULL;

HHOOK hKeyboardHook = NULL;
HHOOK hMouseHook = NULL;

HANDLE hServerThread = NULL;
DWORD dwServerThreadID = 0;

// client

HANDLE hInstance = NULL;

HANDLE hClientThread = NULL;
DWORD dwClientThreadID = 0;

HANDLE hClientPipe = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);
JNIEXPORT void JNICALL Java_redlaboratory_globalinputmanager_GlobalHook_enable(JNIEnv *, jclass);
JNIEXPORT void JNICALL Java_redlaboratory_globalinputmanager_GlobalHook_disable(JNIEnv *, jclass);
DWORD WINAPI hookProc(LPVOID arg);
DWORD WINAPI serverConnectionReceiver(LPVOID arg);
DWORD WINAPI serverClientHandler(LPVOID arg);
DWORD WINAPI clientConnectionRequester(LPVOID arg);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		hInstance = hinstDLL;

		hClientThread = CreateThread(NULL, 0, clientConnectionRequester, NULL, 0, &dwClientThreadID);

		info("[DllMain] DLL_PROCESS_ATTACH: %s, %d\n", getModuleName(), GetCurrentThreadId());
	} else if (fdwReason == DLL_PROCESS_DETACH) {
		CloseHandle(hClientPipe);

		info("[DllMain] DLL_PROCESS_DETACH: %s, %d\n", getModuleName(), GetCurrentThreadId());
	}

	return TRUE;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
	info("[JNI_OnLoad] called.\n");

	pJavaVM = vm;

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_redlaboratory_globalinputmanager_GlobalHook_enable(JNIEnv * env, jclass cls) {
	info("[Java_redlaboratory_globalhook_GlobalHook_00024NativeEventHandler_enable] called.\n");

	hServerThread = CreateThread(NULL, 0, serverConnectionReceiver, NULL, 0, &dwServerThreadID);

	if (hServerThread != NULL) {
		info("[Java_redlaboratory_globalhook_GlobalHook_00024NativeEventHandler_enable] creating thread succeed. %lu\n", dwServerThreadID);
	} else {
		info("[Java_redlaboratory_globalhook_GlobalHook_00024NativeEventHandler_enable] creating thread failed. %lu\n", dwServerThreadID);
	}

	CreateThread(NULL, 0, hookProc, NULL, 0, NULL);
}

JNIEXPORT void JNICALL Java_redlaboratory_globalinputmanager_GlobalHook_disable(JNIEnv * env, jclass cls) {
	info("[Java_redlaboratory_globalhook_GlobalHook_00024NativeEventHandler_disable] called.\n");

	if (hKeyboardHook) {
		UnhookWindowsHookEx(hKeyboardHook);
		UnhookWindowsHookEx(hMouseHook);
		hKeyboardHook = NULL;
	}

	Message msg = {MT_TERMINATE, 0, 0};
	DWORD dwWritten;

	if (WaitNamedPipe(UNC_PIPE_NAME, NMPWAIT_WAIT_FOREVER) == TRUE) {
		HANDLE pipeToTerminate = CreateFile(UNC_PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

		if (pipeToTerminate != INVALID_HANDLE_VALUE) {
			WriteFile(pipeToTerminate, &msg, sizeof(msg), &dwWritten, NULL);
			FlushFileBuffers(pipeToTerminate);
			CloseHandle(pipeToTerminate);
		}
	}

	if (hClientPipe) {
		WriteFile(hClientPipe, &msg, sizeof(msg), &dwWritten, NULL);
		FlushFileBuffers(hClientPipe);
		CloseHandle(hClientPipe);
	}
}

DWORD WINAPI hookProc(LPVOID arg) {
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, 0);// 0 for last parameter means that it will hook for every processes.
	hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, 0);

	if (hKeyboardHook != NULL && hMouseHook != NULL) {
		info("[Java_redlaboratory_globalhook_GlobalHook_enable] hook succeed.\n");
	} else {
		info("[Java_redlaboratory_globalhook_GlobalHook_enable] hook failed.\n");
	}

	MSG msg;
	BOOLEAN bRunThread = TRUE;

	while(bRunThread) {
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

DWORD WINAPI serverConnectionReceiver(LPVOID arg) {
	HANDLE hPipe;
	DWORD threadID;
	BOOL bCon;

	while (TRUE) {
		hPipe = CreateNamedPipe(
			UNC_PIPE_NAME,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE,
			PIPE_UNLIMITED_INSTANCES,
			1024, 1024,
			1000,
			NULL);

		if (hPipe == INVALID_HANDLE_VALUE) {
			info("[serverConnectionReceiver] creating named pipe failed.\n");

			break;
		}

		info("[serverConnectionReceiver] Created named pipe and waiting.\n");

		bCon = ConnectNamedPipe(hPipe, NULL);

		if (bCon == FALSE && GetLastError() == ERROR_PIPE_CONNECTED) bCon = TRUE;

		if (bCon == TRUE) {
			Message msg;
			DWORD dwRead;
			BOOL bSuc = ReadFile(hPipe, &msg, sizeof(msg), &dwRead, NULL);

			CreateThread(NULL, 0, serverClientHandler, (LPVOID) hPipe, 0, &threadID);
			info("[serverConnectionReceiver] Created serverClientHandler for %lu.\n", threadID);

			if (bSuc == TRUE && dwRead > 0) {
				info("[serverConnectionReceiver] Connect accepted. %d, %lu\n", msg.messageType, threadID);

				if (msg.messageType == MT_TERMINATE) break;
			} else {
				info("[serverConnectionReceiver] Connect not accepted. %d\n", msg.messageType);
			}

			info("[serverConnectionReceiver] received connecting request and succeed. %lu\n", threadID);
		} else {
			CloseHandle(hPipe);

			info("[serverConnectionReceiver] received connecting request and failed.\n");
		}
	}

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	info("[serverConnectionReceiver] ended.\n");

	return 0;
}

DWORD WINAPI serverClientHandler(LPVOID arg) {
	JNIEnv* pJNIEnv = NULL;
	(*pJavaVM)->AttachCurrentThread(pJavaVM, (void**) &pJNIEnv, NULL);

	jclass cls = (*pJNIEnv)->FindClass(pJNIEnv, NATIVE_EVENT_CLASS);
	jmethodID keyboardEvent = (*pJNIEnv)->GetStaticMethodID(pJNIEnv, cls, "nativeKeyboardEvent", "(IZZZZZ)Z");
	jmethodID mouseEvent = (*pJNIEnv)->GetStaticMethodID(pJNIEnv, cls, "nativeMouseEvent", "(IIIIS)V");

	HANDLE hPipe = (HANDLE) arg;
	Message msg;
	DWORD dwRead, dwWrite;
	BOOL bSuc;

	while (TRUE) {
		bSuc = ReadFile(hPipe, &msg, sizeof(msg), &dwRead, NULL);
		if (bSuc == FALSE || dwRead <= 0) break;

		info("[serverClientHandler] message received. %lu, %d, %d, %d, %d, %d, %d, %d\n", GetCurrentThreadId(), msg.messageType, msg.released, msg.extendedKey, msg.altDowned, msg.ctrlDowned, msg.shiftDowned, msg.mouseEventType);

		if (msg.messageType == MT_ACCEPT) {
			continue;
		} else if (msg.messageType == MT_TERMINATE) {
			break;
		} else if (msg.messageType == MT_KEYBOARD) {
			jboolean jb = (*pJNIEnv)->CallStaticBooleanMethod(pJNIEnv, cls, keyboardEvent, (jint) msg.vk, (jboolean) msg.extendedKey, (jboolean) msg.released, (jboolean) msg.altDowned, (jboolean) msg.ctrlDowned, (jboolean) msg.shiftDowned);
			info("\n[serverClientHandler] Result is %d, %d, %d, %s.\n", msg.vk, msg.released, jb, keyboardEvent == NULL ? "NULL" : "NOT_NULL");

			bSuc = WriteFile(hPipe, &jb, 1, &dwWrite, NULL);
			if (bSuc == FALSE || dwWrite <= 0) break;
		} else if (msg.messageType == MT_MOUSE) {
			(*pJNIEnv)->CallStaticVoidMethod(pJNIEnv, cls, mouseEvent, (jint) msg.mouseEventType, (jint) msg.mouseButton, (jint) msg.x, (jint) msg.y, (jshort) msg.delta);
		}
	}

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	(*pJavaVM)->DetachCurrentThread(pJavaVM);
	info("[serverClientHandler] terminated. %lu\n", GetCurrentThreadId());

	return 0;
}

DWORD WINAPI clientConnectionRequester(LPVOID arg) {
	info("[clientConnectionRequester] %s has been waiting for server. %lu\n", getModuleName(), GetCurrentThreadId());

	while (TRUE) {
		if (WaitNamedPipe(UNC_PIPE_NAME, NMPWAIT_WAIT_FOREVER) == TRUE) {
			hClientPipe = CreateFile(
					UNC_PIPE_NAME,
					GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);

			if (hClientPipe != INVALID_HANDLE_VALUE) {
				info("[clientConnectionRequester] connected with server. %s, %lu\n", getModuleName(), GetCurrentThreadId());

				Message msg = {MT_ACCEPT, 0, 0};
				DWORD dwWritten;
				BOOL bSuc = WriteFile(hClientPipe, &msg, sizeof(msg), &dwWritten, NULL);

				if (bSuc == TRUE && dwWritten > 0) {
					info("[clientConnectionRequester] send 0 flag message %lu\n", GetCurrentThreadId());
				} else {
					info("[clientConnectionRequester] can't send 0 flag message %lu\n", GetCurrentThreadId());
				}

				break;
			} else {
				info("[clientConnectionRequester] failed to connect with server. %s, %lu\n", getModuleName(), GetCurrentThreadId());
			}
		}
	}

	info("[clientConnectionRequester] ended. %s, %lu\n", getModuleName(), GetCurrentThreadId());

	return 0;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		BOOL released = (lParam >> 30) & 0x1;// 0(FALSE) is for pressed and 1(TRUE) is for released.
		BOOL extendedKey = (lParam >> 24) & 0x1;// 0(FALSE) is for not extended key and 1(TRUE) is for otherwise.
//		BOOL altDowned = (lParam >> 29) & 0x1;// 0(FALSE) is for alt key is down and 1(TRUE) is for otherwise.
		BOOL altDowned = GetAsyncKeyState(VK_MENU) & 0x8000 ? TRUE : FALSE;
		BOOL ctrlDowned = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? TRUE : FALSE;
		BOOL shiftDowned = GetAsyncKeyState(VK_SHIFT) & 0x8000 ? TRUE : FALSE;

		info("[KeyboardProc] message received.: %lu, %d, %d, %d, %s, %lu\n", wParam, released, extendedKey, altDowned, getModuleName(), GetCurrentThreadId());

		if (hClientPipe != NULL) {
			DWORD dwWritten, dwRead;
			BOOL bSuc;

			Message msg = {MT_KEYBOARD, wParam, released, extendedKey, altDowned, ctrlDowned, shiftDowned};

			bSuc = WriteFile(hClientPipe, &msg, sizeof(msg), &dwWritten, NULL);

			if (bSuc == TRUE && dwWritten > 0) {
				byte canceled[1];

				bSuc = ReadFile(hClientPipe, canceled, sizeof(canceled), &dwRead, NULL);
				if (bSuc == FALSE || dwRead <= 0) canceled[0] = JNI_FALSE;

				if (canceled[0] == JNI_TRUE) {
					info("[KeyboardProc] input canceled.\n");

					return 1;
				}
			} else {
				CloseHandle(hClientPipe);
				hClientPipe = NULL;
			}
		}
	}

	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		MOUSEHOOKSTRUCT* mhs = (MOUSEHOOKSTRUCT*) lParam;
		MOUSEHOOKSTRUCTEX* mhsEx = (MOUSEHOOKSTRUCTEX*) lParam;

		info("[MouseProc] message received. : %d, %d\n", mhs->pt.x, mhs->pt.y);

		int lr = -1;
		int mouseEventType = -1;
		short delta = 0;

		if (wParam == WM_LBUTTONDOWN) {
			lr = MOUSE_LEFT;
			mouseEventType = MOUSE_DOWN;
		} else if (wParam == WM_LBUTTONUP) {
			lr = MOUSE_LEFT;
			mouseEventType = MOUSE_UP;
		} else if (wParam == WM_LBUTTONDBLCLK) {
			lr = MOUSE_LEFT;
			mouseEventType = MOUSE_DB_CLICK;
		} else if (wParam == WM_RBUTTONDOWN) {
			lr = MOUSE_RIGHT;
			mouseEventType = MOUSE_DOWN;
		} else if (wParam == WM_RBUTTONUP) {
			lr = MOUSE_RIGHT;
			mouseEventType = MOUSE_UP;
		} else if (wParam == WM_RBUTTONDBLCLK) {
			lr = MOUSE_RIGHT;
			mouseEventType = MOUSE_DB_CLICK;
		} else if (wParam == WM_MOUSEMOVE) {
			lr = MOUSE_MOVE;
			mouseEventType = MOUSE_MOVE;
		} else if (wParam == WM_MBUTTONDOWN) {
			lr = MOUSE_MIDDLE;
			mouseEventType = MOUSE_DOWN;
		} else if (wParam == WM_MBUTTONUP) {
			lr = MOUSE_MIDDLE;
			mouseEventType = MOUSE_UP;
		} else if (wParam == WM_MBUTTONDBLCLK) {
			lr = MOUSE_MIDDLE;
			mouseEventType = MOUSE_DB_CLICK;
		} else if (wParam == WM_MOUSEWHEEL) {
			delta = HIWORD(mhsEx->mouseData);
			lr = MOUSE_MIDDLE;
			mouseEventType = MOUSE_WHEEL_SCROLL;
		} else if (wParam == WM_XBUTTONDOWN) {
			delta = HIWORD(mhsEx->mouseData);
			lr = delta == XBUTTON1 ? MOUSE_XBUTTON1 : MOUSE_XBUTTON2;
			mouseEventType = MOUSE_DOWN;
		} else if (wParam == WM_XBUTTONUP) {
			delta = HIWORD(mhsEx->mouseData);
			lr = delta == XBUTTON1 ? MOUSE_XBUTTON1 : MOUSE_XBUTTON2;
			mouseEventType = MOUSE_UP;
		} else if (wParam == WM_XBUTTONDBLCLK) {
			delta = HIWORD(mhsEx->mouseData);
			lr = delta == XBUTTON1 ? MOUSE_XBUTTON1 : MOUSE_XBUTTON2;
			mouseEventType = MOUSE_DB_CLICK;
		}

		DWORD dwWritten;
		BOOL bSuc;

		Message msg = {MT_MOUSE, 0, 0, 0, 0, 0, 0, mouseEventType, lr, mhs->pt.x, mhs->pt.y, delta};

		bSuc = WriteFile(hClientPipe, &msg, sizeof(msg), &dwWritten, NULL);

		if (bSuc == FALSE || dwWritten <= 0) {
			CloseHandle(hClientPipe);
			hClientPipe = NULL;
		}
	}

	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}
