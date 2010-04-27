#include "StdAfx.h"

#include <Lm.h>

#include "activepet.h"
#include "Syncer.h"
#include "../polaris/auto_array.h"
#include "../polaris/auto_buffer.h"
#include "../polaris/auto_impersonation.h"
#include "../polaris/Constants.h"
#include "../polaris/RegKey.h"
#include "../polaris/Runnable.h"
#include "sendmailhandler.h"
#include "FileEventLoop.h"
#include "MessageDispatcher.h"
#include "../pethooks/inject.h"
#include "../pethooks/pethooks.h"
#include "GetOpenFileNameAHandler.h"
#include "GetOpenFileNameWHandler.h"
#include "GetClipboardDataHandler.h"

auto_buffer<PSID> GetLogonSID(HANDLE token) 
{
    // Determine the needed buffer size.
    DWORD groups_length = 0;
    if (!GetTokenInformation(token, TokenGroups, NULL, 0, &groups_length)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return auto_buffer<PSID>();
        }
    }
    auto_buffer<PTOKEN_GROUPS> ptg(groups_length);

    // Get the token group information from the access token.
    if (!GetTokenInformation(token, TokenGroups, ptg.get(), groups_length, &groups_length)) {
        return auto_buffer<PSID>();
    }

    // Loop through the groups to find the logon SID.
    for (int i = 0; i != ptg->GroupCount; ++i) {
        if ((ptg->Groups[i].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
            DWORD sid_length = GetLengthSid(ptg->Groups[i].Sid);
            auto_buffer<PSID> r(sid_length);
            if (CopySid(sid_length, r.get(), ptg->Groups[i].Sid)) {
                return r;
            }
            break;
        }
    }
    return auto_buffer<PSID>();
}

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES | \
WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | \
WINSTA_READSCREEN | STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid)
{
    // Obtain the DACL for the window station.
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    DWORD sd_length = 0;
    if (!GetUserObjectSecurity(hwinsta, &si, NULL, 0, &sd_length)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
            return FALSE;
        }
    }
    auto_buffer<PSECURITY_DESCRIPTOR> psd(sd_length);
    if (!GetUserObjectSecurity(hwinsta, &si, psd.get(), sd_length, &sd_length)) {
        printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Create a new DACL.
    auto_buffer<PSECURITY_DESCRIPTOR> psd_new(sd_length);
    if (!InitializeSecurityDescriptor(psd_new.get(), SECURITY_DESCRIPTOR_REVISION)) {
        printf("InitializeSecurityDescriptor() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Get the DACL from the security descriptor.
    BOOL bDaclPresent;
    PACL pacl;
    BOOL bDaclExist;
    if (!GetSecurityDescriptorDacl(psd.get(), &bDaclPresent, &pacl, &bDaclExist)) {
        printf("GetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }
    
    // Initialize the ACL.
    ACL_SIZE_INFORMATION aclSizeInfo = {};
    aclSizeInfo.AclBytesInUse = sizeof(ACL);
    if (NULL != pacl) {
        // get the file ACL size info
        if (!GetAclInformation(pacl, &aclSizeInfo, sizeof aclSizeInfo, AclSizeInformation)) {
            printf("GetAclInformation() failed: %d\n", GetLastError());
            return FALSE;
        }
    }

    // Compute the size of the new ACL.
    DWORD new_acl_size = aclSizeInfo.AclBytesInUse +
                         (2 * sizeof(ACCESS_ALLOWED_ACE)) + 
                         (2 * GetLengthSid(psid)) - (2 * sizeof(DWORD));
    auto_buffer<PACL> new_acl(new_acl_size);

    // Initialize the new DACL.
    if (!InitializeAcl(new_acl.get(), new_acl_size, ACL_REVISION)) {
        printf("InitializeAcl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // If DACL is present, copy it to a new DACL.
    if (bDaclPresent) {
        // Copy the ACEs to the new ACL.
        if (aclSizeInfo.AceCount) {
            for (DWORD i = 0; i != aclSizeInfo.AceCount; ++i) {
                LPVOID pTempAce;
                if (!GetAce(pacl, i, &pTempAce)) {
                    printf("GetAce() failed: %d\n", GetLastError());
                    return FALSE;
                }
                if (!AddAce(new_acl.get(), ACL_REVISION, MAXDWORD,
                            pTempAce, ((PACE_HEADER)pTempAce)->AceSize)) {
                    printf("AddAce() failed: %d\n", GetLastError());
                    return FALSE;
                }
            }
        }
    }

    // Add the first ACE to the window station.
    auto_buffer<ACCESS_ALLOWED_ACE*> ace(sizeof(ACCESS_ALLOWED_ACE) +
                                         GetLengthSid(psid) -
                                         sizeof(DWORD));
    ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    ace->Header.AceFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
    ace->Header.AceSize = (WORD) (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD));
    ace->Mask = GENERIC_ACCESS;
    if (!CopySid(GetLengthSid(psid), &ace->SidStart, psid)) {
        printf("CopySid() failed: %d\n", GetLastError());
        return FALSE;
    }
    if (!AddAce(new_acl.get(), ACL_REVISION, MAXDWORD, ace.get(), ace->Header.AceSize)) {
        printf("AddAce() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Add the second ACE to the window station.
    ace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
    ace->Mask = WINSTA_ALL;
    if (!AddAce(new_acl.get(), ACL_REVISION,MAXDWORD, ace.get(), ace->Header.AceSize)) {
        printf("AddAce() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Set a new DACL for the security descriptor.
    if (!SetSecurityDescriptorDacl(psd_new.get(), TRUE, new_acl.get(), FALSE)) {
        printf("SetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Set the new security descriptor for the window station.
    if (!SetUserObjectSecurity(hwinsta, &si, psd_new.get())) {
        printf("SetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}


BOOL RemoveAceFromWindowStation(HWINSTA hwinsta, PSID psid)
{
    // Obtain the DACL for the window station.
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    DWORD sd_length = 0;
    if (!GetUserObjectSecurity(hwinsta, &si, NULL, 0, &sd_length)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
            return FALSE;
        }
    }
    auto_buffer<PSECURITY_DESCRIPTOR> psd(sd_length);
    if (!GetUserObjectSecurity(hwinsta, &si, psd.get(), sd_length, &sd_length)) {
        printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Create a new DACL.
    auto_buffer<PSECURITY_DESCRIPTOR> psd_new(sd_length);
    if (!InitializeSecurityDescriptor(psd_new.get(), SECURITY_DESCRIPTOR_REVISION)) {
        printf("InitializeSecurityDescriptor() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Get the DACL from the security descriptor.
    BOOL bDaclPresent;
    PACL pacl;
    BOOL bDaclExist;
    if (!GetSecurityDescriptorDacl(psd.get(), &bDaclPresent, &pacl, &bDaclExist)) {
        printf("GetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }
    
    // Initialize the ACL.
    ACL_SIZE_INFORMATION aclSizeInfo = {};
    aclSizeInfo.AclBytesInUse = sizeof(ACL);
    if (NULL != pacl) {
        // get the file ACL size info
        if (!GetAclInformation(pacl, &aclSizeInfo, sizeof aclSizeInfo, AclSizeInformation)) {
            printf("GetAclInformation() failed: %d\n", GetLastError());
            return FALSE;
        }
    }

    // Compute the size of the new ACL.
    DWORD new_acl_size = aclSizeInfo.AclBytesInUse -
                         ((2 * sizeof(ACCESS_ALLOWED_ACE)) + 
                          (2 * GetLengthSid(psid)) - (2 * sizeof(DWORD)));
    auto_buffer<PACL> new_acl(new_acl_size);

    // Initialize the new DACL.
    if (!InitializeAcl(new_acl.get(), new_acl_size, ACL_REVISION)) {
        printf("InitializeAcl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // If DACL is present, copy it to a new DACL.
    if (bDaclPresent) {
        // Copy the ACEs to the new ACL.
        for (DWORD i = 0; i != aclSizeInfo.AceCount; ++i) {
            ACCESS_ALLOWED_ACE* pace;
            if (!GetAce(pacl, i, (void**)&pace)) {
                printf("GetAce() failed: %d\n", GetLastError());
                return FALSE;
            }
            if (!EqualSid(psid, &pace->SidStart)) {
                if (!AddAce(new_acl.get(), ACL_REVISION, MAXDWORD,
                            pace, pace->Header.AceSize)) {
                    printf("AddAce() failed: %d\n", GetLastError());
                    return FALSE;
                }
            }
        }
    }

    // Set a new DACL for the security descriptor.
    if (!SetSecurityDescriptorDacl(psd_new.get(), TRUE, new_acl.get(), FALSE)) {
        printf("SetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Set the new security descriptor for the window station.
    if (!SetUserObjectSecurity(hwinsta, &si, psd_new.get())) {
        printf("SetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

BOOL AddAceToDesktop(HDESK hdesk, PSID psid)
{
    // Obtain the security descriptor for the desktop object.
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    DWORD sd_size = 0;
    if (!GetUserObjectSecurity(hdesk, &si, NULL, 0, &sd_size)) {
         if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
            return FALSE;
         }
    }
    auto_buffer<PSECURITY_DESCRIPTOR> psd(sd_size);
    if (!GetUserObjectSecurity(hdesk, &si, psd.get(), sd_size, &sd_size)) {
        printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Create a new security descriptor.
    auto_buffer<PSECURITY_DESCRIPTOR> psd_new(sd_size);
    if (!InitializeSecurityDescriptor(psd_new.get(), SECURITY_DESCRIPTOR_REVISION)) {
        printf("InitializeSecurityDescriptor() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Obtain the DACL from the security descriptor.
    BOOL bDaclPresent;
    PACL pacl;
    BOOL bDaclExist;
    if (!GetSecurityDescriptorDacl(psd.get(), &bDaclPresent, &pacl, &bDaclExist)) {
        printf("GetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Initialize.
    ACL_SIZE_INFORMATION aclSizeInfo = {};
    aclSizeInfo.AclBytesInUse = sizeof(ACL);
    if (pacl != NULL) {
        // Determine the size of the ACL information.
        if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof aclSizeInfo, AclSizeInformation)) {
            printf("GetAclInformation() failed: %d\n", GetLastError());
            return FALSE;
        }
    }

    // Allocate buffer for the new ACL.
    DWORD dwNewAclSize = aclSizeInfo.AclBytesInUse +
                         sizeof(ACCESS_ALLOWED_ACE) +
                         GetLengthSid(psid) - sizeof(DWORD);
    auto_buffer<PACL> new_acl(dwNewAclSize);

    // Initialize the new ACL.
    if (!InitializeAcl(new_acl.get(), dwNewAclSize, ACL_REVISION)) {
        printf("InitializeAcl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // If DACL is present, copy it to a new DACL.
    if (bDaclPresent) {
        // Copy the ACEs to the new ACL.
        if (aclSizeInfo.AceCount) {
            for (DWORD i = 0; i != aclSizeInfo.AceCount; ++i) {
                // Get an ACE.
                PVOID pTempAce;
                if (!GetAce(pacl, i, &pTempAce)) {
                    printf("GetAce() failed: %d\n", GetLastError());
                    return FALSE;
                }

                // Add the ACE to the new ACL.
                if (!AddAce(new_acl.get(), ACL_REVISION,MAXDWORD, pTempAce,
                            ((PACE_HEADER)pTempAce)->AceSize)) {
                    printf("AddAce() failed: %d\n", GetLastError());
                    return FALSE;
                }
            }
        }
    }

    // Add ACE to the DACL.
    if (!AddAccessAllowedAce(new_acl.get(), ACL_REVISION, DESKTOP_ALL, psid)) {
        printf("AddAccessAllowedAce() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Set new DACL to the new security descriptor.
    if (!SetSecurityDescriptorDacl(psd_new.get(), TRUE, new_acl.get(), FALSE)) {
        printf("SetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Set the new security descriptor for the desktop object.
    if (!SetUserObjectSecurity(hdesk, &si, psd_new.get())) {
        printf("SetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }
    
    return TRUE;
}


BOOL RemoveAceFromDesktop(HDESK hdesk, PSID psid)
{
    // Obtain the security descriptor for the desktop object.
    SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
    DWORD sd_size = 0;
    if (!GetUserObjectSecurity(hdesk, &si, NULL, 0, &sd_size)) {
         if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
            return FALSE;
         }
    }
    auto_buffer<PSECURITY_DESCRIPTOR> psd(sd_size);
    if (!GetUserObjectSecurity(hdesk, &si, psd.get(), sd_size, &sd_size)) {
        printf("GetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Create a new security descriptor.
    auto_buffer<PSECURITY_DESCRIPTOR> psd_new(sd_size);
    if (!InitializeSecurityDescriptor(psd_new.get(), SECURITY_DESCRIPTOR_REVISION)) {
        printf("InitializeSecurityDescriptor() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Obtain the DACL from the security descriptor.
    BOOL bDaclPresent;
    PACL pacl;
    BOOL bDaclExist;
    if (!GetSecurityDescriptorDacl(psd.get(), &bDaclPresent, &pacl, &bDaclExist)) {
        printf("GetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Initialize.
    ACL_SIZE_INFORMATION aclSizeInfo = {};
    aclSizeInfo.AclBytesInUse = sizeof(ACL);
    if (pacl != NULL) {
        // Determine the size of the ACL information.
        if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof aclSizeInfo, AclSizeInformation)) {
            printf("GetAclInformation() failed: %d\n", GetLastError());
            return FALSE;
        }
    }

    // Allocate buffer for the new ACL.
    DWORD dwNewAclSize = aclSizeInfo.AclBytesInUse -
                         (sizeof(ACCESS_ALLOWED_ACE) +
                          GetLengthSid(psid) - sizeof(DWORD));
    auto_buffer<PACL> new_acl(dwNewAclSize);

    // Initialize the new ACL.
    if (!InitializeAcl(new_acl.get(), dwNewAclSize, ACL_REVISION)) {
        printf("InitializeAcl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // If DACL is present, copy it to a new DACL.
    if (bDaclPresent) {
        // Copy the ACEs to the new ACL.
        for (DWORD i = 0; i != aclSizeInfo.AceCount; ++i) {
            // Get an ACE.
            ACCESS_ALLOWED_ACE* pace;
            if (!GetAce(pacl, i, (void**)&pace)) {
                printf("GetAce() failed: %d\n", GetLastError());
                return FALSE;
            }
            if (!EqualSid(psid, &pace->SidStart)) {
                // Add the ACE to the new ACL.
                if (!AddAce(new_acl.get(), ACL_REVISION,MAXDWORD, pace,
                            pace->Header.AceSize)) {
                    printf("AddAce() failed: %d\n", GetLastError());
                    return FALSE;
                }
            }
        }
    }

    // Set new DACL to the new security descriptor.
    if (!SetSecurityDescriptorDacl(psd_new.get(), TRUE, new_acl.get(), FALSE)) {
        printf("SetSecurityDescriptorDacl() failed: %d\n", GetLastError());
        return FALSE;
    }

    // Set the new security descriptor for the desktop object.
    if (!SetUserObjectSecurity(hdesk, &si, psd_new.get())) {
        printf("SetUserObjectSecurity() failed: %d\n", GetLastError());
        return FALSE;
    }
    
    return TRUE;
}

ActivePet::ActivePet(wchar_t* user_password,
                     std::wstring petname,
                     FileEventLoop* files) : m_petname(petname),
                                             m_files(files),
                                             m_session(INVALID_HANDLE_VALUE),
                                             m_profile(INVALID_HANDLE_VALUE),
                                             m_job(CreateJobObject(NULL, NULL)),
                                             m_next_editable_name(1),
                                             m_ticks_while_invisible(0)
{
    // Create a new logon session for the pet.
    std::wstring petRegistryPath = Constants::registryPets() + L"\\" + m_petname;
	RegKey petkey = RegKey::HKCU.open(petRegistryPath);
	std::wstring accountName = petkey.getValue(L"accountName");
    std::wstring accountRegistryPath = Constants::registryAccounts() + L"\\" + accountName;
	RegKey accountKey = RegKey::HKCU.open(accountRegistryPath);	
	std::wstring password = accountKey.getValue(L"password");
	if (!LogonUser(accountName.c_str(), NULL, password.c_str(),
				   LOGON32_LOGON_INTERACTIVE,
				   LOGON32_PROVIDER_DEFAULT,
				   &m_session)) {
        printf("LogonUser() failed: %d\n", GetLastError());
        return;
	}

    // Tweak the Winsta0 and desktop ACLs to allow the pet to create windows.
    auto_buffer<PSID> logon_sid = GetLogonSID(m_session.get());
    if (NULL == logon_sid.get()) {
        return;
    }
    auto_close<HWINSTA, &::CloseWindowStation> winsta0(OpenWindowStation(L"winsta0", FALSE, READ_CONTROL | WRITE_DAC));
	if (NULL == winsta0.get()) {
        printf("OpenWindowStation() failed: %d\n", GetLastError());
        return;
	}
	if (!AddAceToWindowStation(winsta0.get(), logon_sid.get())) {
        printf("AddAceToWindowStation() failed: %d", GetLastError());
		return;
	}
    auto_close<HDESK, &::CloseDesktop> desktop(OpenDesktop(L"default", 0, FALSE,
                                                           READ_CONTROL | WRITE_DAC |
                                                           DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS));
	if (NULL == desktop.get()) {
        printf("OpenDesktop() failed: %d\n", GetLastError());
		return;
	}
    if (!AddAceToDesktop(desktop.get(), logon_sid.get())) {
        printf("AddAceToDesktop() failed: %d\n", GetLastError());
		return;
	}

    // Load the pet account's registry hive.
    wchar_t account[128] = {};
    wcsncpy(account, accountName.c_str(), 128);
	PROFILEINFO profile = { sizeof(PROFILEINFO), 0, account };
	if (!LoadUserProfile(m_session.get(), &profile)) {
        printf("LoadUserProfile() failed: %d\n", GetLastError());
		return;
	}
    m_profile = profile.hProfile;

    // Initialize the pet job.
	if (NULL == m_job.get()) {
        printf("CreateJobObject() failed: %d\n", GetLastError());
		return;
	}
    JOBOBJECT_BASIC_UI_RESTRICTIONS buir = { JOB_OBJECT_UILIMIT_HANDLES };
	if (!SetInformationJobObject(m_job.get(), JobObjectBasicUIRestrictions, &buir, sizeof buir)) {
        printf("SetInformationJobObject() failed: %d\n", GetLastError());
	}

    // Some apps fail to launch without access to the desktop window.
	if (!UserHandleGrantAccess(GetDesktopWindow(), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}

    // Give apps access to all the standard cursors.
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_ARROW), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_IBEAM), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_WAIT), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_CROSS), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_UPARROW), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZE), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_ICON), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZENWSE), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZENESW), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZEWE), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZENS), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZEALL), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_NO), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_HAND), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_APPSTARTING), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}
	if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_HELP), m_job.get(), TRUE)) {
        printf("UserHandleGrantAccess() failed: %d\n", GetLastError());
	}

    // Setup the use records for the printers.
    std::set<std::wstring> servers;
    RegKey printers = RegKey::HKCU.open(L"Printers\\Connections");
    for (int i = 0; true; ++i) {
        RegKey printer = printers.getSubKey(i);
        if (!printer.isGood()) {
            break;
        }
        std::wstring server = printer.getValue(L"Server");
        if (servers.count(server) == 0) {

            std::wstring resource = server + L"\\IPC$";
            auto_array<wchar_t> remote(resource.length() + 1);
            lstrcpy(remote.get(), resource.c_str());

            USE_INFO_2 use = {};
            use.ui2_remote = remote.get();
            use.ui2_domainname = _wgetenv(L"USERDOMAIN");
            use.ui2_username = _wgetenv(L"USERNAME");;
            use.ui2_password = user_password;
            use.ui2_asg_type = USE_WILDCARD;

            auto_impersonation impersonate(m_session.get());

            DWORD arg_error;
            NET_API_STATUS error = NetUseAdd(NULL, 2, (BYTE*)&use, &arg_error);
            if (error) {
                printf("NetUseAdd() failed: %d\n", error);
            } else {
                servers.insert(server);
            }
        }
    }

    // Add the message handlers.
    //m_requestFolderPath = accountKey.getValue(Constants::petDataPathName()) + Constants::requestPath();
    m_requestFolderPath = Constants::requestsPath(Constants::polarisDataPath(Constants::userProfilePath(accountName)));
    m_dispatcher = new MessageDispatcher(m_requestFolderPath);
    m_files->watch(m_requestFolderPath, m_dispatcher);
    m_dispatcher->add("sendmail",makeSendMailHandler());
// TODO    m_dispatcher->add("GetOpenFileNameA", makeGetOpenFileNameAHandler(this));
    m_dispatcher->add("GetOpenFileNameW", makeGetOpenFileNameWHandler(this));
	m_dispatcher->add("GetClipboardData", makeGetClipboardDataHandler());
}

ActivePet::~ActivePet()
{
    m_files->ignore(m_requestFolderPath, m_dispatcher);
    // Kill any stray processes.
    if (!TerminateJobObject(m_job.get(), -1)) {
        printf("TerminateJobObject() failed: %d", GetLastError());
    }

    // TODO: Change this algorithm for transient accounts.
    std::for_each(m_syncers.begin(), m_syncers.end(), delete_element<Syncer*>());

    RegKey editables = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname + L"\\" + Constants::editables());
    editables.removeSubKeys();

    // Unload the profile.
    UnloadUserProfile(m_session.get(), m_profile);

    // Undo the changes to the winsta0 and desktop DACLs.
    auto_buffer<PSID> logon_sid = GetLogonSID(m_session.get());
    if (NULL == logon_sid.get()) {
        return;
    }
    auto_close<HWINSTA, &::CloseWindowStation> winsta0(OpenWindowStation(L"winsta0", FALSE, READ_CONTROL | WRITE_DAC));
	if (NULL == winsta0.get()) {
        printf("OpenWindowStation() failed: %d", GetLastError());
        return;
	}
	if (!RemoveAceFromWindowStation(winsta0.get(), logon_sid.get())) {
        printf("RemoveAceFromWindowStation() failed: %d", GetLastError());
		return;
	}
    auto_close<HDESK, &::CloseDesktop> desktop(OpenDesktop(L"default", 0, FALSE,
                                                           READ_CONTROL | WRITE_DAC |
                                                           DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS));
	if (NULL == desktop.get()) {
        printf("OpenDesktop() failed: %d", GetLastError());
		return;
	}
    if (!RemoveAceFromDesktop(desktop.get(), logon_sid.get())) {
        printf("AddAceToDesktop() failed: %d", GetLastError());
		return;
	}
}

bool ActivePet::ownsProcess(HANDLE process) {
	BOOL result;
	if (!IsProcessInJob(process, m_job.get(), &result)) {
        printf("IsProcessInJob() failed: %d", GetLastError());
	}
    return result ? true : false;
}

std::wstring ActivePet::mirror(std::wstring originalDirPath) {

	// Create a pet local copy of the original file.
	RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname);
	std::wstring accountName = petkey.getValue(L"accountName");
	RegKey accountKey = RegKey::HKCU.open(Constants::registryAccounts() + L"\\" + accountName);
    std::wstring editableDirPath = Constants::editablesPath(Constants::polarisDataPath(Constants::userProfilePath(accountName)));
    CreateDirectory(editableDirPath.c_str(), NULL);

    // create a new folder under the editables folder to hold the editable file
    std::wstring::size_type i = 0;
    std::wstring::size_type j = originalDirPath.find(':');
    if (j != std::wstring::npos && i != j) {
        editableDirPath += L"\\" + originalDirPath.substr(i, j - i);
        CreateDirectory(editableDirPath.c_str(), NULL);
        i = ++j;
    } else {
        j = 0;
    }
    while (i != originalDirPath.length()) {
        j = originalDirPath.find('\\', i);
        if (j == std::wstring::npos) {
            break;
        }
        if (i != j) {
            editableDirPath += L"\\" + originalDirPath.substr(i, j - i);
            CreateDirectory(editableDirPath.c_str(), NULL);
        }
        i = ++j;
    }

    return editableDirPath;
}

std::wstring ActivePet::edit(std::wstring originalDocPath) {

    // Check if the pet already has a local copy of the original file.
	std::wstring editableDocPath;
	RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname);
    RegKey editables = petkey.open(Constants::editables());
	for (int i = 0; true; ++i) {
		RegKey live = editables.getSubKey(i);
		if (!live.isGood()) {
			break;
		}
		if (originalDocPath == live.getValue(L"originalPath")) {
			editableDocPath = live.getValue(L"editablePath");
			break;
		}
	}
    // if there is not already a local copy of the original file
	if (editableDocPath == L"") {

        editableDocPath = mirror(originalDocPath) + originalDocPath.substr(originalDocPath.find_last_of('\\'));

        // TODO: This seemingly harmless loop will go forever if the registry has
        // been trashed (in particular, if the editables key does not exist)
        // Should we make a long random name with StrongPassword? I think yes.
		// Remember that we have this stray file lying around.
        while (true) {
            wchar_t buffer[16] = {};
            _itow(m_next_editable_name++, buffer, 10);
		    RegKey entry = editables.create(buffer);
		    if (entry.isGood()) {
		        entry.setValue(L"originalPath", originalDocPath);
		        entry.setValue(L"editablePath", editableDocPath);
                break;
		    }
        }

        // now copy the file 
		if (!CopyFile(originalDocPath.c_str(), editableDocPath.c_str(), TRUE)) {
            printf("CopyFile() failed: %d\n", GetLastError());
		} 

        // set up the syncer
		Syncer* syncer = new Syncer(m_files, editableDocPath, originalDocPath);
		m_syncers.push_back(syncer);
	}

    return editableDocPath;
}

int ActivePet::launch(std::wstring exePath, std::wstring originalDocPath){

	wchar_t commandLine[4096] = { '\0' };
	if (originalDocPath != L"") {

		// Build the command line.
	    std::wstring editableDocPath = edit(originalDocPath);
		wcscat(commandLine, L"\"");
		wcscat(commandLine, exePath.c_str());
		wcscat(commandLine, L"\" \"");
		wcscat(commandLine, editableDocPath.c_str());
		wcscat(commandLine, L"\"");
	}
    auto_close<void*, &::DestroyEnvironmentBlock> environment(NULL);
	if (!CreateEnvironmentBlock(&environment, m_session.get(), FALSE)) {
        printf("CreateEnvironmentBlock() failed: %d", GetLastError());
		return -30;
	}
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi = {};
	if (!CreateProcessAsUser(m_session.get(),
							 exePath.c_str(),
							 commandLine,
							 NULL,
							 NULL,
							 FALSE,
							 CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE,
							 environment.get(),
							 NULL, // lpCurrentDirectory,
							 &si,
							 &pi)) {
        printf("CreateProcessAsUser() failed: %d", GetLastError());
		return -30;
	}
	if (!AssignProcessToJobObject(m_job.get(), pi.hProcess)) {
        printf("AssignProcessToJobObject() failed: %d", GetLastError());
		return -30;
	}
	if (!ResumeThread(pi.hThread)) {
        printf("ResumeThread() failed: %d", GetLastError());
		return -30;
	}
    if (WaitForInputIdle(pi.hProcess, INFINITE) != ERROR_SUCCESS) {
        printf("WaitForInputIdle() failed: %d", GetLastError());
		return -30;
	}
    InjectAndCall(pi.hProcess, "ApplyHooks", 10000);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

    return 0;
}

bool ActivePet::age() {
    return ++m_ticks_while_invisible >= MAX_TICKS_WHILE_INVISIBLE;
}

void ActivePet::resetAge() {
    m_ticks_while_invisible = 0;
}
