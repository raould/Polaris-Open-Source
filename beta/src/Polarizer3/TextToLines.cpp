#include "stdafx.h"
#include "TextToLines.h"

std::vector<std::wstring> textToLines(std::wstring textIn) {
    std::wstring text = textIn;
    std::vector<std::wstring> lines;
    while (true) {
        size_t index = text.find(L"\r\n");
        if (index == std::string.npos) {
            if (text.length() > 0) {
                lines.push_back(text);
            }
            break;
        } else {
            std::wstring nextLine = text.substr(0,index);
            size_t truncatedCount = index + 2;
            std::wstring remnant = text.substr(truncatedCount, text.length() - truncatedCount);
            text = remnant;
            lines.push_back(nextLine);
        }
    }
    return lines;
}

std::vector<std::wstring> textWidgetToLines(CEdit* widget) {
    CString chars;
    widget->GetWindowText(chars);
    std::wstring text = (const wchar_t*) chars;
    return textToLines(text);
}