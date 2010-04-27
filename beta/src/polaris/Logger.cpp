// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "logger.h"
#include "Constants.h"

Logger::Logger(std::wstring messageProlog) : m_messageProlog(messageProlog)
{
}

Logger::~Logger()
{
}

int Logger::basicLog(std::wstring message){
	//XXX build the logfile location from the polaris folder, not app folder
	//std::string appdata(getenv("APPDATA"));
	//std::string logpath = appdata  + Constants::relativePath() + "\\log.txt";
    std::wstring logpath = Constants::polarisDataPath(_wgetenv(L"USERPROFILE")) + L"\\log.txt";
	OutputDebugString(message.c_str());
	OutputDebugString(L"\n");
    int errorcode;
    FILE* logger = _wfopen(logpath.c_str(), L"a+");
    if (logger) {
	    errorcode = fwprintf(logger, L"%s\n", message.c_str());
	    fclose(logger);
    } else {
        errorcode = 0;
    }
	return errorcode > 0 ? 0 : errorcode;
}

int Logger::log(std::wstring message) const
{
	return Logger::basicLog(m_messageProlog + L" - " + message);
}


int Logger::log(std::wstring message, int errorNumber) const {
    wchar_t buffer[1000];
    return Logger::basicLog(m_messageProlog + L" - " + message + L" error: " + _itow(errorNumber, buffer, 10));
}