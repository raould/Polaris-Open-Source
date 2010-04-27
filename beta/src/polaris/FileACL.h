// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class FileACL {
    std::wstring m_filePath;
public:
    FileACL(std::wstring filePath);
    virtual ~FileACL(void);
    static std::wstring READ() {return L":r";}
    static std::wstring WRITE() {return L":w";}
    static std::wstring MODIFY() {return L":m";}
    static std::wstring FULL() {return L":f";}
	static std::wstring EXECUTE() {return L":x";}
    static std::wstring POLARIS_USERS() {return L"Users";}
    void grant(std::wstring account, std::wstring rights);
    void revoke(std::wstring account);
    void wipe();
	bool immediateReadGrant(std::wstring account);
};
