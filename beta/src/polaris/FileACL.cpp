// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include ".\FileACL.h"
#include <shellapi.h>
#include "Logger.h"
#include "Constants.h"
#include <Aclapi.h>
#include <Sddl.h>


Logger aclLogger(L"FileACL");

std::wstring enquoter(std::wstring text) {return L"\"" + text + L"\"";}

void launchXcacls(std::wstring command) {
    std::wstring xcaclsPath = Constants::polarisExecutablesFolderPath() + L"\\xcacls.vbs";
    int success = (int)ShellExecute(NULL, NULL, L"cscript", (enquoter(xcaclsPath) + L" " + command).c_str(), NULL, SW_HIDE);
    if (success <= 32) {aclLogger.log(L"xcacls.vbs did not launch successfully");}
	
}


FileACL::FileACL(std::wstring filePath){
    m_filePath = filePath;
}

FileACL::~FileACL(void)
{
}

/**
 * The grant is eventual, i.e., asynchronous
 */
void FileACL::grant(std::wstring account, std::wstring rights){
    launchXcacls(enquoter(m_filePath) + L"  /e /g " + account + rights);
}

/**
 * the revocation is eventual, i.e., asynchronous
 */
void FileACL::revoke(std::wstring account){
    launchXcacls(enquoter(m_filePath) + L"  /e /r " + account);
}

/**
 * the wipe is eventual
 */
void FileACL::wipe(){
    launchXcacls(enquoter(m_filePath) + L"  /g Administrators:f");
}

/**
 * This is not robust, in that it simply adds a new ace for the account to the end of the dacl.
 *   If there is already an ace for this account in this dacl, the result is undetermined
 * Assumes the account being set is local, not a domain account, i.e., assumes it is a pet account
 *
 * Algorithm: Get the current security descriptor for the file, get the sid for the account,
 * convert the security descriptor to a string, create a new stringified ACE for this account,
 * append the new ace string to the security descriptor string, convert the security descriptor
 * string to a relative security descriptor, convert the relative security descriptor to an 
 * absolute descriptor, extract a pointer to the new dacl in the absolute descriptor, update the dacl
 * portion of the file's security descriptor with the new dacl.
 *   
 **/
bool FileACL::immediateReadGrant(std::wstring account) {

	//get the original security descriptor as a string
	PSECURITY_DESCRIPTOR originalSecurityDescriptor = NULL;
	PACL originalDacl;
	DWORD getInfoResult = GetNamedSecurityInfo(
		// Why do I have to cast this? Is this an error waiting to happen?
		(LPWSTR)m_filePath.c_str(),
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		&originalDacl,
		NULL,
		&originalSecurityDescriptor);
	if (getInfoResult != ERROR_SUCCESS) {
		aclLogger.log(L"getnamedsecurityinfo failed",getInfoResult);
		return false;
	}
	ULONG securityDescriptorLen;
	LPTSTR stringSecurityDescriptor;
	BOOL convertToStringSuccess = ConvertSecurityDescriptorToStringSecurityDescriptor(
		originalSecurityDescriptor,
		SDDL_REVISION_1,
		DACL_SECURITY_INFORMATION,
		&stringSecurityDescriptor,
		&securityDescriptorLen);
	
	LocalFree(originalSecurityDescriptor);
	if (!convertToStringSuccess) {
		aclLogger.log(L"convertSD to string failed", GetLastError());
		return false;
	}
	std::wstring secDesc = stringSecurityDescriptor;
	LocalFree(stringSecurityDescriptor);

	// get the SID for the account in string form
	int const sidMaxLen = 1000;
	BYTE sid[sidMaxLen] = {};
	DWORD sidSize = sidMaxLen;
	SID_NAME_USE peUse;
	DWORD domainSize = sidMaxLen;
	wchar_t domain[sidMaxLen] = {};
	BOOL gettingSIDsuccess =  LookupAccountName(
		NULL,
		account.c_str(),
		sid,
		&sidSize,
		domain,
		&domainSize,
		&peUse
		);
	if (!gettingSIDsuccess) {
		aclLogger.log(L"lookupaccountname failed", GetLastError());
		LocalFree(originalSecurityDescriptor);
		LocalFree(stringSecurityDescriptor);
		return false;
	}
	LPTSTR sidNameChars;
	BOOL convertedSIDtoString = ConvertSidToStringSid(sid, &sidNameChars);
	std::wstring accountSid = sidNameChars;
	LocalFree(sidNameChars);

	//manufacture new string ace in format:
	// ace_type;ace_flags;rights;object_guid;inherit_object_guid;account_sid
	std::wstring newAce = L"(A;OICI;GRFR;;;" + accountSid + L")";

	//manufacture the new dacl security description
	std::wstring newDaclString = secDesc + newAce;
	PSECURITY_DESCRIPTOR newRelativeSecurityDescriptor;
	ULONG sdSize;
	BOOL convertStringSDtoSDsuccess = ConvertStringSecurityDescriptorToSecurityDescriptor(
		newDaclString.c_str(),
		SDDL_REVISION_1,
		&newRelativeSecurityDescriptor,
		&sdSize);
	if (!convertStringSDtoSDsuccess) {
		aclLogger.log(L"convertstringsd to sd failed", GetLastError());
		return false;
	}

	//transform relative sd to absolute sd so we can extract the dacl
	DWORD const outputSize = 5000;
	DWORD absoluteSDSize = outputSize;
	BYTE absoluteSD[outputSize] = {};
	DWORD daclSize = outputSize;
	BYTE dacl[outputSize] = {};
	DWORD paclSize = outputSize;
	BYTE pacl[outputSize] = {};
	DWORD ownerSize = outputSize;
	BYTE owner[outputSize] = {};
	DWORD groupSize = outputSize;
	BYTE group[outputSize] = {};

	BOOL makeAbsoluteSuccess = MakeAbsoluteSD(
		//  PSECURITY_DESCRIPTOR pSelfRelativeSD,
		newRelativeSecurityDescriptor,
		//  PSECURITY_DESCRIPTOR pAbsoluteSD,
		absoluteSD,
		//  LPDWORD lpdwAbsoluteSDSize,
		&absoluteSDSize,
		//  PACL pDacl,
		(PACL)dacl,
		//  LPDWORD lpdwDaclSize,
		&daclSize,
		//  PACL pSacl,
		(PACL)pacl,
		//  LPDWORD lpdwSaclSize,
		&paclSize,
		//  PSID pOwner,
		(PSID)owner,
		//  LPDWORD lpdwOwnerSize,
		&ownerSize,
		//  PSID pPrimaryGroup,
		(PSID)group,
		//  LPDWORD lpdwPrimaryGroupSize
		&groupSize);
	LocalFree(newRelativeSecurityDescriptor);
	if (!makeAbsoluteSuccess) {
		aclLogger.log(L"makeabsolutesd failed", GetLastError());
		return false;
	}

	DWORD setSecInfoSuccess = SetNamedSecurityInfo(
		//  LPTSTR pObjectName,
		(LPWSTR)m_filePath.c_str(),
		//  SE_OBJECT_TYPE ObjectType,
		SE_FILE_OBJECT,
		//  SECURITY_INFORMATION SecurityInfo,
		DACL_SECURITY_INFORMATION,
		//  PSID psidOwner,
		NULL,
		//  PSID psidGroup,
		NULL,
		//  PACL pDacl,
		(PACL)dacl,
		//  PACL pSacl
		NULL
		);
	if (setSecInfoSuccess != ERROR_SUCCESS) {
		aclLogger.log(L"setsecurityinfo failed", setSecInfoSuccess);
		return false;
	}

	return true;
}
