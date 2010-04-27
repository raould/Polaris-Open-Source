// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include <vector>

void dprintf(char* format, ...);
void titleprintf(HWND window, char* format, ...);
std::vector<std::wstring> Split(std::wstring s, std::wstring separator);
std::wstring Join(std::vector<std::wstring> pieces, std::wstring separator);
std::wstring Trim(std::wstring s);
std::wstring ToWString(std::string s);
std::string ToString(std::wstring s);
bool StartsWith(std::wstring target, std::wstring prefix);
bool EndsWith(std::wstring target, std::wstring prefix);
bool Contains(std::wstring target, std::wstring substr);