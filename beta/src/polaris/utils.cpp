// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "utils.h"
#include <stdio.h>

using namespace std;

// Print a debug message to the debug console and to c:\debug.log.
void dprintf(char* format, ...) {
#ifdef _DEBUG
	char message[4096];
	va_list args;
	va_start(args, format);
	_vsnprintf(message, 4000, format, args);
	va_end(args);
	OutputDebugStringA(message);
	FILE* fp = fopen("c:\\debug.log", "a");
	if (fp) {
		fprintf(fp, "%s", message);
		fclose(fp);
	}
#endif
}

// Display a debug message in the title of a window and print it to c:\debug.log.
void titleprintf(HWND window, char* format, ...) {
#ifdef _DEBUG
	char message[4096];
	va_list args;
	va_start(args, format);
	_vsnprintf(message, 4000, format, args);
	va_end(args);
	SetWindowTextA(window, message);
	FILE* fp = fopen("c:\\debug.log", "a");
	if (fp) {
		fprintf(fp, "%s\n", message);
		fclose(fp);
	}
#endif
}

vector<wstring> Split(wstring s, wstring separator) {
	vector<wstring> pieces;
	size_t start = 0;
	while (true) {
		size_t end = s.find(separator, start);
		if (end == string::npos) break;
		pieces.push_back(s.substr(start, end - start));
		start = end + 1;
	}
	pieces.push_back(s.substr(start));
	return pieces;
}

wstring Join(vector<wstring> pieces, wstring separator) {
	wstring result;
	bool first = true;
	for (vector<wstring>::iterator it = pieces.begin(); it != pieces.end(); it++) {
		if (!first) result += separator;
		first = false;
		result += (*it);
	}
	return result;
}

wstring Trim(wstring s) {
	size_t left = s.find_first_not_of(L" \t\r\n");
	size_t right = s.find_last_not_of(L" \t\r\n");
	if (left == wstring::npos) left = 0;
	if (right == wstring::npos) right = s.length() - 1;
	return s.substr(left, 1 + right - left);
}

wstring ToWString(string s) {
	const char* p = s.c_str();
	int len = (int) s.length();
	int wlen = MultiByteToWideChar(CP_ACP, 0, p, len, NULL, 0);
	if (wlen == 0) return wstring(L"");
	wstring result;
	result.resize(wlen);
	MultiByteToWideChar(CP_ACP, 0, p, len, (wchar_t*) result.c_str(), wlen);
	return result;
}

string ToString(wstring ws) {
	const wchar_t* wp = ws.c_str();
	int wlen = (int) ws.length();
	int len = WideCharToMultiByte(CP_ACP, 0, wp, wlen, NULL, 0, NULL, NULL);
	if (len == 0) return string("");
	string result;
	result.resize(len);
	WideCharToMultiByte(CP_ACP, 0, wp, wlen, (char*) result.c_str(), len, NULL, NULL);
	return result;
}

bool StartsWith(wstring target, wstring prefix) {
	return target.length() >= prefix.length() &&
		target.substr(0, prefix.length()) == prefix;
}

bool EndsWith(wstring target, wstring suffix) {
	return target.length() >= suffix.length() &&
		target.substr(target.length() - suffix.length()) == suffix;
}

bool Contains(wstring target, wstring substr) {
	return target.find(substr) != wstring.npos;
}