// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "RegKey.h"
#include "Logger.h"
#include <shlwapi.h>

RegKey RegKey::HKCU(HKEY_CURRENT_USER);
RegKey RegKey::HKU(HKEY_USERS);
RegKey RegKey::HKLM(HKEY_LOCAL_MACHINE);
RegKey RegKey::HKR(HKEY_CLASSES_ROOT);
RegKey RegKey::INVALID(NULL);

RegKey::RegKey(HKEY key) : m_key(key), m_refcount(new int)
{
	*m_refcount = 1;
}

RegKey::RegKey(const RegKey& x) : m_key(x.m_key), m_refcount(x.m_refcount)
{
	++(*m_refcount);
}

RegKey::~RegKey()
{
	if (0 == --(*m_refcount)) {
		delete m_refcount;
        if (isGood()) {
		    LONG error = RegCloseKey(m_key);
		    if (ERROR_SUCCESS != error) {
			    printf("RegCloseKey() failed: %d\n", error);
		    }
        }
	}
}


std::wstring RegKey::NO_VALUE_FOUND() {return L"No Value Found";}

bool RegKey::isGood() const
{
	return m_refcount != INVALID.m_refcount;
}


DWORD RegKey::getType(std::wstring valueName) {
    DWORD type;
    DWORD size = 512;
    BYTE value[512];
    LONG error = RegQueryValueEx(m_key, valueName.c_str(), NULL, &type, value, &size);
    return type;
}

/**
 * returns NO_VALUE_FOUND if no value identified. Returns "" for the default value
 */
std::wstring RegKey::getValueName(int i) {
    wchar_t valueName[512];
    DWORD size = 512;
    LONG err = RegEnumValue(
        m_key,
        i,
        valueName,
        &size,
        NULL,
        NULL,
        NULL,
        NULL);
    if (err == ERROR_SUCCESS) {
        return valueName;
    } else {
        return RegKey::NO_VALUE_FOUND();
    }

}

std::wstring RegKey::getValue(std::wstring name) const {
	DWORD type;
	DWORD size = 512;
	BYTE value[512];
    std::wstring r;
	LONG error = RegQueryValueEx(m_key, name.c_str(), NULL, &type, value, &size);
    if (type != REG_EXPAND_SZ && type != REG_SZ) {
        Logger::basicLog(L"RegKey retrieval of nonstring as string");
        //not a string variable
        return L"";
    }
    switch (error) {
    case ERROR_SUCCESS:
	    r = std::wstring((const wchar_t*)value);
        break;
    case ERROR_MORE_DATA:
        {
            BYTE* big_value = new BYTE[size];
            r = std::wstring((const wchar_t*)big_value);
            delete[] big_value;
        }
        break;
    default:
		printf("RegQueryValueEx() failed: %d\n", error);
        r = L"";
    }
    return r;
}

bool RegKey::getBytesValue(std::wstring name, LPBYTE binary, LPDWORD size) const {
	DWORD type;
	LONG error = RegQueryValueEx(m_key, name.c_str(), NULL, &type, binary, size);
    switch (error) {
    case ERROR_SUCCESS:
	    return true;
    case ERROR_MORE_DATA:
        Logger::basicLog(L"RegKey binary retrieval, retrieval array too small");
        return false;
    default:
		printf("RegQueryValueEx() failed: %d\n", error);
        return false;
    }
}


void RegKey::setValue(std::wstring name, std::wstring value)
{
	LONG error = RegSetValueEx(m_key, name.c_str(), 0, REG_SZ, 
                               (const BYTE*)value.c_str(), (DWORD)((value.length() + 1) * sizeof(wchar_t)));
	if (ERROR_SUCCESS != error) {
		printf("RegSetValueEx() failed: %d\n", error);
	}
}

void RegKey::setBytesValue(std::wstring name, DWORD type, LPBYTE binary, DWORD size)
{
	LONG error = RegSetValueEx(m_key, name.c_str(), 0, type, 
                               binary, size);
	if (ERROR_SUCCESS != error) {
		printf("RegSetValueEx() failed: %d\n", error);
    }
}

RegKey RegKey::open(std::wstring subpath) {
	HKEY key;
	LONG error = RegOpenKey(m_key, subpath.c_str(), &key);
    // interestingly, the subkey "" returns error_success as status, but 
    // an attempt later to close the key handle fails
	if (ERROR_SUCCESS != error || subpath.length() == 0) {
		wprintf(L"RegOpenKey() failed: %s %d\n", subpath.c_str(), error);
        return INVALID;
	}
	return RegKey(key);
}

RegKey RegKey::create(std::wstring child)
{
	HKEY key;
	DWORD disposition;
	LONG error = RegCreateKeyEx(m_key, child.c_str(), 0, NULL,
                                 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                                 &key, &disposition);
	if (ERROR_SUCCESS != error || child.length() == 0) {
		printf("RegCreateKeyEx() failed: %d\n", error);
        return INVALID;
	}
	return RegKey(key);
}

// Registry constants from: <http://msdn.microsoft.com/library/default.asp?url=/library/en-us/sysinfo/base/registry_element_size_limits.asp>
const int MAX_KEY_NAME_LENGTH = 256;

RegKey RegKey::getSubKey(int i)
{
    std::wstring subKeyName = getSubKeyName(i);
    if (subKeyName.length() == 0) {return INVALID;}
    return open(subKeyName.c_str());
}

std::wstring RegKey::getSubKeyName(int i) {
	TCHAR name[MAX_KEY_NAME_LENGTH];
	DWORD name_length = MAX_KEY_NAME_LENGTH;
	FILETIME last_modified;
    LONG error = RegEnumKeyEx(m_key, i, name, &name_length, NULL, NULL, NULL, &last_modified);
    switch (error) {
    case ERROR_SUCCESS:
        return name;
    case ERROR_NO_MORE_ITEMS:
        return L"";
    default:
		printf("RegEnumKeyEx() failed: %d\n", error);
        return L"";
    }
}

void RegKey::removeSubKeys() {
    while (true) {
	    wchar_t name[MAX_KEY_NAME_LENGTH];
	    DWORD name_length = MAX_KEY_NAME_LENGTH;
	    FILETIME last_modified;
        LONG error = RegEnumKeyEx(m_key, 0, name, &name_length, NULL, NULL, NULL, &last_modified);
	    if (ERROR_SUCCESS == error) {
            deleteSubKey(name);
        } else if (ERROR_NO_MORE_ITEMS == error) {
            break;
        } else {
		    printf("RegEnumKeyEx() failed: %d\n", error);
            Logger::basicLog(L"RegEnumKeyEx() failed in removeSubKeys");
            break;
        }
    }
}

void RegKey::deleteSubKey(std::wstring subKeyName) {
    LONG delete_error = SHDeleteKey(m_key, subKeyName.c_str());
	if (ERROR_SUCCESS != delete_error) {
		printf("RegDeleteKey() failed: %d\n", delete_error);
        Logger::basicLog(L"RegKey deletion failed for " + subKeyName);
    }
}



