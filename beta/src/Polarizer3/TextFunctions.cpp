// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "TextFunctions.h"

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

/**
 * copies a list of lines into a single textarea, separating items with cr-lf
 */
void linesToWidget(std::vector<std::wstring> lines, CEdit* widget) {
    std::wstring concat;
    for (size_t i = 0; i < lines.size(); ++i) {
        concat = concat + lines[i] + L"\r\n";
    }
    widget->SetWindowText(concat.c_str());
}