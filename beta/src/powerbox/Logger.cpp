#include "StdAfx.h"
#include "logger.h"

Logger::Logger(std::string messageProlog) : m_messageProlog(messageProlog)
{
}

Logger::~Logger()
{
}

int Logger::basicLog(std::string message){
	//XXX build the logfile location from the polaris folder, not app folder
	std::string appdata(getenv("APPDATA"));
	std::string logpath = appdata  + "\\hp\\polaris\\log.txt";
    FILE* logger = fopen(logpath.c_str(), "a+");
	int errorcode = fprintf(logger, "%s\n",message.c_str());
	if (errorcode >0 ) {errorcode =0;};
	fclose(logger);
	return errorcode;
}

int Logger::log(std::string message)
{
	return Logger::basicLog(m_messageProlog + " - " + message);
}