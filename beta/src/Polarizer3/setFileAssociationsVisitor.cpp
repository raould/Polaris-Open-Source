// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "../polaris/Constants.h"
#include ".\SetFileassociationsvisitor.h"

SetFileAssociationsVisitor::SetFileAssociationsVisitor(std::wstring checkedSuffix, 
																 std::wstring appPath, 
																 std::wstring petname){
	m_appPath = appPath;
	m_petname = petname;
	m_suffix = checkedSuffix;
	std::wstring softwareClasses = L"Software\\Classes\\";
    RegKey testSuffixKey = RegKey::HKLM.open(softwareClasses + L"." + m_suffix);
    if (!testSuffixKey.isGood()) {
        RegKey tempSuffixKey = RegKey::HKLM.create(softwareClasses + L"." + m_suffix);
        m_progIDstring = Constants::polarisBaseName() + L"." + m_suffix + L".1";
        tempSuffixKey.setValue(L"", m_progIDstring);
    } else {
        std::wstring candidateProgIDstring = testSuffixKey.getValue(L"");
        RegKey hklmProgID = RegKey::HKLM.open(softwareClasses + candidateProgIDstring);
        RegKey curVerKey = hklmProgID.open(L"CurVer");
        if (curVerKey.isGood()) {
            m_progIDstring = curVerKey.getValue(L"");
        } else {
            m_progIDstring = candidateProgIDstring;
        }
    }
}

SetFileAssociationsVisitor::~SetFileAssociationsVisitor(void)
{
}

// TODO: put enquote as a function in the polaris dll, so we don't have a bunch
// of functions with almost the same name doing exactly the same thing
std::wstring enquoter2(std::wstring text) {return L"\"" + text + L"\"";}

/*
 * given the root of a user account, create a default launch behavior for a particular 
 * suffix: the progIDstring is the "type" represented by a particular suffix that
 * must be ascertained by looking in the HKLM registry before coming here
 */
void setFileProgID(RegKey user, std::wstring suffix, std::wstring progIDstring, 
                   std::wstring appPath, std::wstring petname) {
                       
    std::wstring softwareClasses = L"Software\\Classes\\";
    RegKey testProgIDkey = user.open(softwareClasses + progIDstring);
    if (!testProgIDkey.isGood()) {
        RegKey tempProgIDkey = user.create(softwareClasses + progIDstring);
        tempProgIDkey.setValue(L"", suffix + L" File");
    }
    RegKey progIDkey = RegKey::HKCU.open(softwareClasses + progIDstring);
    RegKey shellKey = progIDkey.create(L"shell");
    shellKey.setValue(L"", L"OpenSafe");
    RegKey openSafeKey = shellKey.create(L"OpenSafe");
    RegKey commandKey = openSafeKey.create(L"command");
    commandKey.setValue(L"", enquoter2(Constants::polarisExecutablesFolderPath() + 
        L"\\powercmd.exe") + L" launch " +  enquoter2(petname) + L" " + enquoter2(appPath) + L" " + enquoter2(L"%1"));
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

void SetFileAssociationsVisitor::visit(RegKey pet) {
    setFileProgID(pet, m_suffix, m_progIDstring, m_appPath, m_petname);
    
    //RegKey suffixesKey = RegKey::HKCU.create(Constants::registrySuffixes());
    //RegKey suffixKey = suffixesKey.create(suffix);
    //suffixKey.setValue(Constants::registrySuffixAppName(), appPath);
    //suffixKey.setValue(Constants::registrySuffixPetname(), petname);
}




/**
 * removes the progID association in the users software classes, removes the association
 * in the polaris area. 
 *    suffix must not have a preceding "."
 */
//void unSetFileAssociation(std::wstring suffix) {
//    std::wstring softwareClasses = L"Software\\Classes\\";
//    RegKey HKLMsuffixKey = RegKey::HKLM.open(softwareClasses + L"." + suffix);
//    std::wstring progIDstring = HKLMsuffixKey.getValue(L"");
//    RegKey classesKey = RegKey::HKCU.open(softwareClasses);
//    classesKey.deleteSubKey(progIDstring);
//    
//    RegKey suffixesKey = RegKey::HKCU.open(Constants::registrySuffixes());
//    suffixesKey.deleteSubKey(suffix);
//}