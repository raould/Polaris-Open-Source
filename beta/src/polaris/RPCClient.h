// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include "Request.h"

class RPCClient {
    std::wstring m_dir;

public:
    RPCClient(std::wstring dir);
    ~RPCClient();

    Request* initiate(const char* verb, size_t size);
};

RPCClient* makeClient();
