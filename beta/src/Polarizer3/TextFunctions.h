// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

std::vector<std::wstring> textToLines(std::wstring text);

std::vector<std::wstring> textWidgetToLines(CEdit* widget);

void linesToWidget(std::vector<std::wstring> lines, CEdit* widget);