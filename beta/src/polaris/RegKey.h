// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class RegKey
{
	HKEY m_key;
	int* m_refcount;

    // Forbidden operators.
	RegKey(HKEY key);
	RegKey& operator=(const RegKey& x);

public:
	RegKey(const RegKey& x);
	~RegKey();

    /**
     * The HKEY_CURRENT_USER registry key.
     */
	static RegKey HKCU;

    static RegKey HKLM;

    static RegKey HKU;

    static RegKey HKR;

    std::wstring NO_VALUE_FOUND();

    /**
     * The invalid registry key.
     */
    static RegKey INVALID;

    /**
     * Is this a valid registry key?
     */
	bool isGood() const;


    /**
    * returns a registry value type, REG_SZ or REG_BINARY etc. 
    */
    DWORD getType(std::wstring valueName);
    
    /**
     * returns name of next value. If the index is out of range, returns
     * RegKey::NO_VALUE_FOUND. Returns "" for default value at 0
     */
    std::wstring getValueName(int i);

	/**
	 * @return returns a zero length string if an error occurs
	 */
	std::wstring getValue(std::wstring name) const;

    /**
     * returns true if the value retrieved. if the value is not retrieved,
     * because the value name does not exist or the size of the retrieval array
     * (the "binary" variable, whose size is specified in the size variable)
     * is too small.
     */ 
    bool getBytesValue(std::wstring name, LPBYTE binary, LPDWORD size) const;

    /**
     * Sets a value.
     * @param name  The value name.
     * @param value The value data.
     */
	void setValue(std::wstring name, std::wstring value);

    void setBytesValue(std::wstring name, DWORD type, LPBYTE binary, DWORD size);

    /**
     * Opens a sub key.
     * @param path  The sub key name.
     * @return The open sub key.
     */
	RegKey open(std::wstring path);

    /**
     * Creates a new sub key.
     * @param child The sub key name.
     * @return The open sub key.
     */
	RegKey create(std::wstring child);

    /**
     * Gets the ith sub key.
     * @param i The zero based key index.
     * @return The open sub key.
     */
	RegKey getSubKey(int i);

    /**
     * Iteratively deletes all sub-keys, even if the subkeys themselves contain subkeys.
     * Actual deletion of each individual key only occurs when all the handles to the key are closed.
     */
    void removeSubKeys();

    /**
     * deletion only performed when all handles to the deleted key are closed.
     */
    void deleteSubKey(std::wstring subKeyName);

    /**
     * Get the name of this key (not the full path)
     */
    std::wstring getSubKeyName(int i);

    
};
