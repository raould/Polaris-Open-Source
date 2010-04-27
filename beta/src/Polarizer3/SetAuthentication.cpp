// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

// SetAuthentication.cpp : implementation file
//

#include "stdafx.h"
#include "Polarizer3.h"
#include "SetAuthentication.h"
#include ".\setauthentication.h"
#include "..\polaris\RegKey.h"
#include "..\polaris\Constants.h"
#include "..\polaris\Logger.h"
#include <Wincrypt.h>


// SetAuthentication dialog

IMPLEMENT_DYNAMIC(SetAuthentication, CDialog)
SetAuthentication::SetAuthentication(CWnd* pParent /*=NULL*/)
	: CDialog(SetAuthentication::IDD, pParent)
    , noAuthenticationButton(0)
{
}

const int  noAuthenticationSet = 0;
const int atBootAuthenticationSet = 1;
const int storedAuthenticationSet = 2;
int currentAuthenticationSetting = noAuthenticationSet;
SetAuthentication::~SetAuthentication()
{
}

void SetAuthentication::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDCANCEL, setPasswordAtBoot);
	DDX_Control(pDX, IDC_EDIT1, password);
	DDX_Control(pDX, IDC_EDIT2, password2);
}


BEGIN_MESSAGE_MAP(SetAuthentication, CDialog)
    ON_BN_CLICKED(IDC_RADIO1, OnBnClickedRadio1)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_RADIO3, OnBnClickedRadio3)
END_MESSAGE_MAP()

BOOL SetAuthentication::OnInitDialog() {
	CDialog::OnInitDialog();
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
    bool needsPassword = polarisKey.getValue(Constants::registryNeedsPassword()) == L"true";
    bool passwordIsStored = polarisKey.getValue(Constants::registryPasswordIsStored()) == L"true";
	int buttonToCheck = IDC_RADIO1;
	if (needsPassword && passwordIsStored) { 
		buttonToCheck = IDC_RADIO3;
		currentAuthenticationSetting = storedAuthenticationSet;
	}
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, buttonToCheck);
	return true;
}

/**
 * this is the radio button for "No Authentication"
 */

void SetAuthentication::OnBnClickedRadio1()
{
    currentAuthenticationSetting = noAuthenticationSet;
}

/**
 * This is the Set Authentication Button being clicked
 */
void SetAuthentication::OnBnClickedOk()
{
    Logger logger(L"SetAuthentication");
    RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
	std::wstring passFileName = Constants::passwordFilePath(Constants::polarisDataPath(_wgetenv(L"USERPROFILE")));
	DeleteFile(passFileName.c_str());
    FILE* passwordFile = _wfopen(passFileName.c_str(), L"wb");
    // theoretically, the following commented request should tell me which button is selected, but it 
    // always returns zero
    if (currentAuthenticationSetting == noAuthenticationSet) {
        polarisKey.setValue(Constants::registryNeedsPassword(), L"false");
        polarisKey.setValue(Constants::registryPasswordIsStored(), L"false");        
        fwprintf(passwordFile, L"%s", L"") ;
    } else if (currentAuthenticationSetting == atBootAuthenticationSet) {
        polarisKey.setValue(Constants::registryNeedsPassword(), L"true");
        polarisKey.setValue(Constants::registryPasswordIsStored(), L"false");        
        fwprintf(passwordFile, L"%s", L"") ; 
    } else if (currentAuthenticationSetting == storedAuthenticationSet) {
        polarisKey.setValue(Constants::registryNeedsPassword(), L"true");
        polarisKey.setValue(Constants::registryPasswordIsStored(), L"true");
        wchar_t passChars[1000] = {};
        password.GetWindowText(passChars, 900);
		wchar_t pass2Chars[1000] = {};
		password2.GetWindowText(pass2Chars, 900);
		if (wcscmp(passChars, pass2Chars) != 0) {
			MessageBox(L"Please Re-Enter the password in both boxes", L"Passwords do not match", MB_OK);
			return;
		} else {
			DATA_BLOB clearPass;
			DATA_BLOB cryptPass;
			clearPass.pbData = (byte*)passChars;
			clearPass.cbData = wcslen(passChars) *2 + 2;
			if (CryptProtectData(&clearPass, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &cryptPass)) {
				DATA_BLOB testDecrypt;
				CryptUnprotectData(&cryptPass, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &testDecrypt);
				wchar_t testpass [1000] = {};
				wcsncpy(testpass,(wchar_t*) testDecrypt.pbData, testDecrypt.cbData);
				std::wstring teststr = testpass;
				if (teststr != passChars) {
					logger.log(L"password decryption does not match precryption");
				}
				size_t outputCount = fwrite(cryptPass.pbData, 1, cryptPass.cbData, passwordFile);
				LocalFree(cryptPass.pbData);
				if (outputCount < cryptPass.cbData) {
					logger.log(L"Cryptdata fwrite did not write out all chars");
				}

			} else {
				int err = GetLastError();
				logger.log(L"CryptProtectData failed", err);
			}
			//fwprintf(passwordFile, L"%s", passChars) ;
		}
	}
	fclose(passwordFile);
    OnOK();
}

/*
 * the stored-the-password setting radio button
 */
void SetAuthentication::OnBnClickedRadio3()
{
    currentAuthenticationSetting = storedAuthenticationSet;
}
