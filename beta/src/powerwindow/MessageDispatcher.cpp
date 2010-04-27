// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "MessageDispatcher.h"

#include "EventLoop.h"
#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Logger.h"
#include "../polaris/Message.h"
#include "../polaris/Runnable.h"

#include <shlobj.h>

class OutputResponseFile : public MessageResponder
{
    std::wstring m_response_path;

public:
    OutputResponseFile(std::wstring response_path) : m_response_path(response_path) {}
    ~OutputResponseFile() {}

    virtual void run(size_t size, const void* buffer) {
        std::wstring response_file_tmp = m_response_path + L".tmp";
        HANDLE out = CreateFile(response_file_tmp.c_str(), FILE_APPEND_DATA, 0,
                                NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE != out) {
            DWORD written = 0;
            WriteFile(out, buffer, size, &written, NULL);
            CloseHandle(out);
            MoveFile(response_file_tmp.c_str(), m_response_path.c_str());
		}
        delete this;
    }
};

class MessageDispatcherImpl : public MessageDispatcher
{
    EventLoop* m_event_loop;
	std::wstring m_dir;

    Logger m_logger;
    HANDLE m_event;
	typedef std::map<std::string, MessageHandler*> HandlerMap;
	HandlerMap m_handlers;

    class Dispatch : public Runnable {

        MessageDispatcherImpl& m;

    public:
        Dispatch(MessageDispatcherImpl& outer) : m(outer) {}
        virtual ~Dispatch() {}

        virtual void run() {
            bool done = true;
            WIN32_FIND_DATA request = {};
	        std::wstring pattern = m.m_dir + L"\\*.request";
            HANDLE search = FindFirstFile(pattern.c_str(), &request);
            if (INVALID_HANDLE_VALUE != search) {
                do {
		            std::wstring request_path = m.m_dir + L"\\" + request.cFileName;
                    HANDLE in = CreateFile(request_path.c_str(), FILE_READ_DATA, FILE_SHARE_READ,
                                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		            if (INVALID_HANDLE_VALUE != in) {
                        std::wstring request_filename = request.cFileName;
		                size_t dot = request_filename.find('.');
		                std::wstring response_filename =
                            request_filename.substr(0, dot + 1) + L"response";
		                std::wstring response_path = m.m_dir + L"\\" + response_filename;
                        MessageResponder* out = new OutputResponseFile(response_path);
                        DWORD size = GetFileSize(in, NULL);
                        if (INVALID_FILE_SIZE != size) {
                            // Help guard against unterminated strings by ensuring that any string in
                            // the buffer is null terminated before the end of the allocated buffer.
                            BYTE* buffer = new BYTE[size + 3];  // 1 + sizeof(wchar_t)
                            buffer[size] = 0;
                            buffer[size + 1] = 0;
                            buffer[size + 2] = 0;

                            DWORD read = 0;
                            if (ReadFile(in, buffer, size, &read, NULL)) {
                                const char* verb = (const char*)buffer;
                                Message request(buffer, strlen(verb) + 1, read);
                                HandlerMap::iterator entry = m.m_handlers.find(verb);
			                    if (entry != m.m_handlers.end()) {
			                        (*entry).second->run(request, out);
                                    out = NULL;
                                }
                            } else {
                                delete[] buffer;
                            }
                        }
                        CloseHandle(in);
                        if (out) {
                            out->run(0, NULL);
                        }
                        DeleteFile(request_path.c_str());
                    } else {
                        if (ERROR_SHARING_VIOLATION == GetLastError()) {
                            done = false;
                        }
                        m.m_logger.log(L"CreateFile", GetLastError());
                    }
                } while (FindNextFile(search, &request));
                FindClose(search);
            }
            if (done) {
	            FindNextChangeNotification(m.m_event);
            }
        }
    };
    Dispatch* m_dispatch;
public:
	MessageDispatcherImpl(EventLoop* event_loop,
                          std::wstring dir) : m_event_loop(event_loop),
                                              m_dir(dir),
                                              m_logger(L"MessageDispatcher<" + dir + L">")
    {
	    m_event = FindFirstChangeNotification(m_dir.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
        if (INVALID_HANDLE_VALUE == m_event && ERROR_PATH_NOT_FOUND == GetLastError()) {
            SHCreateDirectoryEx(NULL, m_dir.c_str(), NULL);
	        m_event = FindFirstChangeNotification(m_dir.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
        }
        m_dispatch = new Dispatch(*this);
        if (INVALID_HANDLE_VALUE != m_event) {
            m_event_loop->watch(m_event, m_dispatch);
            m_logger.log(L"Serving...");
        } else {
            m_logger.log(L"FindFirstChangeNotification()", GetLastError());
        }
    }
    virtual ~MessageDispatcherImpl() {
        m_event_loop->ignore(m_event, m_dispatch);
        FindCloseChangeNotification(m_event);
        delete m_dispatch;
        std::for_each(m_handlers.begin(), m_handlers.end(), delete_value<HandlerMap::value_type>());
        m_logger.log(L"done");
    }

    virtual void serve(const char* verb, MessageHandler* handler) {
	    m_handlers[verb] = handler;
    }
};

MessageDispatcher* makeMessageDispatcher(EventLoop* event_loop, std::wstring dir) {
    return new MessageDispatcherImpl(event_loop, dir);
}
