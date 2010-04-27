#pragma once
#include <string>

class Logger
{
	std::string m_messageProlog;

public:
	Logger(std::string messageProlog);
	virtual ~Logger();
	/**
	 * Logs messages to a log file in the polaris folder.
	 * @param message the text to append to the log
	 * @return 0 if successful, error code otherwise
	 */
	static int basicLog(std::string message);
	int log(std::string message);
};
