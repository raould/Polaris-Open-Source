// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "FileRequest.h"

FileRequest::FileRequest(const std::wstring& dir,
					 	 const std::wstring& id,
						 size_t promised) : m_dir(dir),
						 					m_id(id),
											Request(promised) {
    m_out = CreateFile((m_dir + L"\\" + m_id + L".request.temp").c_str(),
                       FILE_APPEND_DATA, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
}

FileRequest::~FileRequest(void) {
}

Message FileRequest::finish(bool pump, double timeout) {
    assert(m_written == m_promised);
	DWORD written = 0;
	WriteFile(m_out, m_buffer, (DWORD) m_written, &written, NULL);

    // Signal completion of the request file.
    CloseHandle(m_out);
    HANDLE event = FindFirstChangeNotification(m_dir.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    std::wstring request_path = m_dir + L"\\" + m_id + L".request";
    MoveFile((m_dir + L"\\" + m_id + L".request.temp").c_str(), request_path.c_str());

    // Wait for a reply.
    std::wstring response_path = m_dir + L"\\" + m_id + L".response";
    HANDLE in = INVALID_HANDLE_VALUE;
    time_t start = time(0);
	for (time_t elapsed = 0; INVALID_HANDLE_VALUE == in && elapsed < timeout; elapsed = time(0) - start) {
        DWORD max_wait = (DWORD) ((timeout - elapsed) * 1000);
        DWORD error = pump
            ? MsgWaitForMultipleObjects(1, &event, FALSE, max_wait, QS_ALLINPUT)
            : WaitForSingleObject(event, max_wait);
        if (WAIT_OBJECT_0 == error) {
	        FindNextChangeNotification(event);
            do {
                in = CreateFile(response_path.c_str(), FILE_READ_DATA, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            } while (INVALID_HANDLE_VALUE == in && ERROR_SHARING_VIOLATION == GetLastError());
        } else if (WAIT_OBJECT_0 + 1 == error) {
	        MSG msg;
		    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			    DispatchMessage(&msg);
		    }
        } else if (WAIT_FAILED == error) {
            break;
        }
	}
    FindCloseChangeNotification(event);

	// Read the response.
    Message r;
    if (INVALID_HANDLE_VALUE != in) {
        DWORD size = GetFileSize(in, NULL);
        if (INVALID_FILE_SIZE != size) {
            // Help guard against unterminated strings by ensuring that any string in the received
            // buffer is null terminated before the end of the allocated buffer.
            BYTE* buffer = new BYTE[size + 3];  // 1 + sizeof(wchar_t)
            buffer[size] = 0;
            buffer[size + 1] = 0;
            buffer[size + 2] = 0;

            DWORD read = 0;
            if (ReadFile(in, buffer, size, &read, NULL)) {
                r = Message(buffer, 0, read);
            } else {
                delete[] buffer;
            }
        }
        CloseHandle(in);
        DeleteFile(response_path.c_str());
    } else {
        DeleteFile(request_path.c_str());
    }

    delete this;
    return r;
}
