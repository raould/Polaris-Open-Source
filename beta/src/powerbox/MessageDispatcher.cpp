#include "StdAfx.h"
#include "MessageDispatcher.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/auto_handle.h"
#include "../polaris/conversion.h"

MessageDispatcher::MessageDispatcher(std::wstring folderpath) : m_folderpath(folderpath)
{
}

MessageDispatcher::~MessageDispatcher()
{
}

class OutputResponseFile : public MessageResponder
{
    std::wstring m_response_path;

public:
    OutputResponseFile(std::wstring response_path) : m_response_path(response_path) {}
    ~OutputResponseFile() {}

    void run(int error, const std::vector<std::string>& args) {
        std::wstring response_file_tmp = m_response_path + L".tmp";
		FILE* response_file = _wfopen(response_file_tmp.c_str(), L"a");
		if (response_file) {
            char buffer[12] = {};
			fputs(itoa(error, buffer, 10), response_file);
			fputc('\n', response_file);
			std::vector<std::string>::const_iterator last = args.end();
			for (std::vector<std::string>::const_iterator i = args.begin(); i != last; ++i) {
				fputs((*i).c_str(), response_file);
				fputc('\n', response_file);
			}
			fclose(response_file);
			_wrename(response_file_tmp.c_str(), m_response_path.c_str()); 
		}
        delete this;
    }
};

void MessageDispatcher::run()
{
    WIN32_FIND_DATA request = {};
	std::wstring pattern = m_folderpath + L"\\*.request";
    auto_close<HANDLE, &::FindClose> search(FindFirstFile(pattern.c_str(), &request));
	if (INVALID_HANDLE_VALUE != search.get()) {

		std::wstring filepath = m_folderpath + L"\\" + request.cFileName;
		FILE* file = _wfopen(filepath.c_str(), L"r");
		while (!file) {
			// For some reason, the request file was not ready to be read. Try again.
			printf("fopen() failed as expected when request file not ready to be read\n");
			file = _wfopen(filepath.c_str(), L"r");
		}

		// Read in the string list.
		std::vector<std::string> msg;
        std::string line;
        while (ReadLine(file, &line) == ERROR_SUCCESS) {
            msg.push_back(line);
            line = "";
        }

		// Delete the request file.
		fclose(file);
		if (!DeleteFile(filepath.c_str())) {
			printf("Failed to delete file: %d\n", GetLastError());
		}

		// Process the message.
		std::wstring request_filename = request.cFileName;
		size_t dot = request_filename.find('.');
		std::wstring response_filename = request_filename.substr(0, dot + 1) + L"response";
		std::wstring response_path = m_folderpath + L"\\" + response_filename;
        MessageResponder* responder = new OutputResponseFile(response_path);
		if (msg.size() < 2) {
            responder->run(-4, std::vector<std::string>());
		} else {
            char* verb;
            int pid;
            Deserialize(msg, "si", &verb, &pid);
			HandlerMap::iterator entry = m_handlers.find(verb);
            delete[] verb;
			if (entry == m_handlers.end()) {
                responder->run(-5, std::vector<std::string>());
            } else {
			    std::vector<std::string> args(msg.begin() + 2, msg.end());
			    (*entry).second->run(pid, args, responder);
            }
		}
	}
}
	
void MessageDispatcher::add(std::string verb, MessageHandler* handler){
	m_handlers[verb] = handler;
}

void MessageDispatcher::remove(std::string verb){
	m_handlers.erase(verb);
}
