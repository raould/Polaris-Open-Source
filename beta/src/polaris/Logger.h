// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class Logger
{
	std::wstring m_messageProlog;

public:
	Logger(std::wstring messageProlog);
	virtual ~Logger();
	/**
	 * Logs messages to a log file in the polaris folder.
	 * @param message the text to append to the log
	 * @return 0 if successful, error code otherwise
	 */
	static int basicLog(std::wstring message);
	int log(std::wstring message) const;
    int log(std::wstring message, int errorNumber) const;
};
