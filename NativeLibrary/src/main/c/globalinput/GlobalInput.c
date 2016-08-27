/*
 * Main.c
 *
 *  Created on: 2016. 8. 2.
 *      Author: Áö¼ö
 */

#include <windows.h>
#include "redlaboratory_globalinputmanager_GlobalInput.h"

JNIEXPORT void JNICALL Java_redlaboratory_globalinputmanager_GlobalInput_typeString(JNIEnv *, jclass, jstring);
void typeUnicodeString(int length, PWORD uniStr);
void init(PINPUT pInput);

JNIEXPORT void JNICALL Java_redlaboratory_globalinputmanager_GlobalInput_typeString(JNIEnv * env, jclass cls, jstring str) {
	const jchar * uniStr = (*env)->GetStringChars(env, str, NULL);
	jsize uniStrLength = (*env)->GetStringLength(env, str);

	if (uniStr == NULL) return;

	typeUnicodeString(uniStrLength, (const PWORD) uniStr);

	(*env)->ReleaseStringChars(env, str, uniStr);
}

void typeUnicodeString(int length, const PWORD uniStr) {
	int i;
	for (i = 0; i < length; i++) {
		INPUT input;
		init(&input);
		input.ki.wScan = uniStr[i];

		input.ki.dwFlags = KEYEVENTF_UNICODE;
		SendInput(1, &input, sizeof(INPUT));

		input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));

	}
}

void init(PINPUT pInput) {
	pInput->type = INPUT_KEYBOARD;
	pInput->ki.time = 0;
	pInput->ki.dwExtraInfo = 0;
	pInput->ki.wVk = 0;
}
