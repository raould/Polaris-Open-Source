// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "RPCClient.h"
#include "FileRequest.h"
#include "Constants.h"

RPCClient::RPCClient(std::wstring dir) : m_dir(dir)
{
}	

RPCClient::~RPCClient() {
}

Request* RPCClient::initiate(const char* verb, size_t size)
{
    // Generate an identifier unique to this host for a transaction.
	static wchar_t id[50];
	static int serial = 0;
	SYSTEMTIME t;
	GetSystemTime(&t);
	wsprintfW(id, L"%04d%02d%02d-%02d%02d%02d-%08x-%08x",
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond,
		GetCurrentProcessId(), serial);
	serial++;

    FileRequest* r = new FileRequest(m_dir, id, strlen(verb) + 1 + size);
    r->send(verb, strlen(verb) + 1);
    return r;
}

RPCClient* makeClient() {
	return new RPCClient(Constants::requestsPath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE"))));
}