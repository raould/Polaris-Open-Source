// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "ActivePet.h"

#include "EchoHandler.h"
#include "EventThread.h"
#include "GetClipboardDataHandler.h"
#include "GetOpenFileNameAHandler.h"
#include "GetSaveFileNameAHandler.h"
#include "GetOpenFileNameWHandler.h"
#include "GetSaveFileNameWHandler.h"
#include "MAPISendMailHandler.h"
#include "MessageDispatcher.h"
#include "OpenPetURLHandler.h"
#include "OpenPetStrictHandler.h"
#include "TaskQueue.h"
#include "../pethooks/inject.h"
#include "../polaris/auto_array.h"
#include "../polaris/auto_buffer.h"
#include "../polaris/auto_handle.h"
#include "../polaris/auto_impersonation.h"
#include "../polaris/auto_privilege.h"
#include "../polaris/Constants.h"
#include "../polaris/FileACL.h"
#include "../polaris/Logger.h"
#include "../polaris/RegKey.h"
#include "../polaris/Runnable.h"
#include "../polaris/StrongPassword.h"
#include "../polaris/accountCanOpenFile.h"
#include "../polaris/CopyDirectory.h"

#include <Wincrypt.h>
#include <Lm.h>

/**
 * Grant read authority immediately, no spawn of a process. Then
 * create a bootaction to revoke this grant. Strongpassword used merely to create 
 * a collision-free name
 */
void grantReadWithRevoke(std::wstring account, std::wstring path) {
		FileACL acl(path);
		acl.immediateReadGrant(account);
        RegKey bootActions = RegKey::HKCU.open(Constants::registryBootActions());
        RegKey nextKey = bootActions.create(strongPassword());
        nextKey.setValue(Constants::actionRevokeAccount(), path);
        nextKey.setValue(Constants::accountName(), account);
}

class HookPet : public Runnable 
{
    HANDLE m_process;

    Logger m_logger;

public:
    HookPet(HANDLE process) : m_process(NULL), m_logger(L"HookPet") {
        if (!::DuplicateHandle(::GetCurrentProcess(), process,
                               ::GetCurrentProcess(), &m_process,
                               0, FALSE, DUPLICATE_SAME_ACCESS)) {
            m_logger.log(L"DuplicateHandle()", GetLastError());
        }
    }
    virtual ~HookPet() {
        CloseHandle(m_process);
    }

    // Runnable interface.

    virtual void run() {
        /*
        m_logger.log(L"WaitForInputIdle()...");
        DWORD error = WaitForInputIdle(m_process, 6 * 1000);   // TODO: fix this.
        if (error == ERROR_SUCCESS) {
            m_logger.log(L"WaitForInputIdle() done");
        } else if (WAIT_FAILED == error ) {
            m_logger.log(L"WaitForInputIdle()", GetLastError());
        } else if (WAIT_TIMEOUT == error) {
            m_logger.log(L"WaitForInputIdle() timed out");
        } else {
            m_logger.log(L"WaitForInputIdle() unknown return", GetLastError());
        }
        */
        auto_privilege se_debug(SE_DEBUG_NAME);
	    InjectAndCall(m_process, "ApplyHooks", 10000, NULL, 0, NULL);
    }
};

class RunAccountTask : public Runnable
{
    ActiveAccount& m_account;
    ActiveAccount::Task* m_task;

public:
    RunAccountTask(ActiveAccount& account,
                   ActiveAccount::Task* task) : m_account(account),
                                                m_task(task) {}
    virtual ~RunAccountTask() {
        delete m_task;
    }

    virtual void run() { m_task->run(m_account); }
};

class ActivePetImpl : public ActivePet
{
	std::wstring m_petname;
    BYTE* m_user_sid;
    HANDLE m_job;
    Scheduler* m_scheduler;
    ActiveAccount& m_account;

    Logger m_logger;
    unsigned int m_processes;

public:
    ActivePetImpl(std::wstring petname,
                  BYTE* user_sid,
                  HANDLE job,
                  Scheduler* scheduler,
                  ActiveAccount& account) : m_petname(petname),
                                            m_user_sid(user_sid),
                                            m_job(job),
                                            m_scheduler(scheduler),
                                            m_account(account),
                                            m_logger(petname),
                                            m_processes(0) {}
    virtual ~ActivePetImpl() {
        delete m_user_sid;
    }

    virtual std::wstring getName() const { return m_petname; }
    virtual PSID getUserSID() const { return (PSID)m_user_sid; }

    virtual bool ownsProcess(HANDLE process) const {
	    BOOL result = FALSE;
	    IsProcessInJob(process, m_job, &result);
        return result != FALSE;
    }

    virtual bool manage(HANDLE process) {
        BOOL r = AssignProcessToJobObject(m_job, process);
        if (!r) {
            if (!IsProcessInJob(process, NULL, &r)) {
                m_logger.log(L"IsProcessInJob()", GetLastError());
            }
        }
	    if (r) {
            m_logger.log(m_petname + L" is managing process", GetProcessId(process));
            ++m_processes;
            m_scheduler->send(new HookPet(process));
        }
        return r != FALSE;
    }

    virtual bool release(HANDLE process) {
        return 1 == --m_processes;
    }

    virtual void send(ActiveAccount::Task* task) {
        m_scheduler->send(new RunAccountTask(m_account, task));
    }
};

auto_buffer<PSID> GetLogonSID(HANDLE token) 
{
    // Determine the needed buffer size.
    DWORD groups_length = 0;
    if (!GetTokenInformation(token, TokenGroups, NULL, 0, &groups_length)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return auto_buffer<PSID>();
        }
    }

    // Get the token group information from the access token.
    auto_buffer<PTOKEN_GROUPS> ptg(groups_length);
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

class ActiveAccountImpl : public ActiveAccount
{
    std::wstring m_petname;
    std::wstring m_account_name;
    HANDLE m_job;
    std::wstring m_editable_dir;
    std::wstring m_request_dir;
    EventLoop* m_event_loop;
    Scheduler* m_scheduler;

    Logger m_logger;
	HANDLE m_zombie;
    HANDLE m_session;
    MessageDispatcher* m_dispatcher;

	unsigned int m_next_editable_name;

    /**
     * Setup use records for the specified servers.
     */
    class UseServers : public Runnable 
    {
        ActiveAccountImpl& m;
        wchar_t m_user_password[1000];

    
        void addImpersonationForServer(std::wstring server) {
            std::wstring resource = server + L"\\IPC$";
            auto_array<wchar_t> remote(resource.length() + 1);
            lstrcpy(remote.get(), resource.c_str());

            USE_INFO_2 use = {};
            use.ui2_remote = remote.get();
            use.ui2_domainname = _wgetenv(L"USERDOMAIN");
            use.ui2_username = _wgetenv(L"USERNAME");;
            use.ui2_password = m_user_password;
            use.ui2_asg_type = USE_WILDCARD;


            DWORD arg_error;
            NET_API_STATUS error;
            {
                auto_impersonation impersonate(m.m_session);
                error = NetUseAdd(NULL, 2, (BYTE*)&use, &arg_error);
            }
            if (NERR_Success != error) {
                if (ERROR_LOGON_FAILURE == error) {
                    std::wstring msg = L"Failed to authenticate to: " + resource;
                    MessageBox(NULL,
                               msg.c_str(),
                               L"Polaris network authentication failure",
                               MB_OK);
                } else {
                    m.m_logger.log(L"NetUseAdd(\"" + resource +
                        L"\", \"" + use.ui2_domainname +
                        L"\", \"" + use.ui2_username +
                        L"\", \"" + use.ui2_password + L"\")", error);
                }
            }
        }

    public:
        UseServers(ActiveAccountImpl& outer,
			wchar_t* user_password) : m(outer) {
            wcscpy(m_user_password, user_password);
											 
		}
        virtual ~UseServers() {}

        // Runnable interface.

        virtual void run() {
		    // Setup the use records for the printers.
		    std::set<std::wstring> alreadyAuthenticatedServers;
		    RegKey printers = RegKey::HKCU.open(L"Printers\\Connections");
		    for (int i = 0; true; ++i) {
			    RegKey printer = printers.getSubKey(i);
			    if (!printer.isGood()) {
				    break;
			    }
			    std::wstring server = printer.getValue(L"Server");
			    if (alreadyAuthenticatedServers.count(server) == 0) {
				    alreadyAuthenticatedServers.insert(server);
				    addImpersonationForServer(server);
			    }
		    }
		    // Setup use records for additional servers specified for this pet
		    RegKey customAuthenticatedServers = RegKey::HKCU.open(Constants::registryPets() + L"\\" 
			    + m.m_petname + L"\\" + Constants::serversToAuthenticate());
		    for (int i = 0; true; ++i) {
			    RegKey authenticatedServerKey = customAuthenticatedServers.getSubKey(i);
			    if (!authenticatedServerKey.isGood()) {break;}
			    std::wstring server = authenticatedServerKey.getValue(L"");
			    if (alreadyAuthenticatedServers.count(server) == 0) {
				    alreadyAuthenticatedServers.insert(server);
				    addImpersonationForServer(server);
			    }
		    }
        }
    };

    class Logon : public Runnable
    {
        ActiveAccountImpl& m;
        wchar_t m_user_password[1000];

    public:
        Logon(ActiveAccountImpl& outer,
              wchar_t* user_password) : m(outer) {
			wcscpy(m_user_password, user_password);	
			// get the password if it is stored; getting it here ensures that, if the user changes 
			// passwords in the middle of a session, future launches of pets will pick up the 
			// new password
			RegKey polarisKey = RegKey::HKCU.open(Constants::registryBase());
			bool needsPassword = polarisKey.getValue(Constants::registryNeedsPassword()) == L"true";
			bool passwordStored = polarisKey.getValue(Constants::registryPasswordIsStored()) == L"true";
			if (needsPassword && passwordStored) {
				FILE* passwordFile = _wfopen(Constants::passwordFilePath(Constants::polarisDataPath(
															_wgetenv(L"USERPROFILE"))).c_str(), L"rb");
					DATA_BLOB clearPass;
					DATA_BLOB cryptPass;
					byte cryptBytes[1000];
					size_t bytesSize = fread(cryptBytes,1, 900, passwordFile); 
					cryptPass.pbData = cryptBytes;
					cryptPass.cbData = bytesSize;
					if (CryptUnprotectData(&cryptPass, NULL, NULL, NULL, NULL,
										CRYPTPROTECT_UI_FORBIDDEN, &clearPass)) {
						wcscpy(m_user_password, (wchar_t*) clearPass.pbData);
						LocalFree(clearPass.pbData);
					} else {
						Logger localLogger(L"activepet.make");
						localLogger.log(L"cryptUnprotectData failed", GetLastError());
					}
				//fgetws(password, sizeof(password) -1, passwordFile);
				fclose(passwordFile);
			}
		}
        virtual ~Logon() {}

        virtual void run() {
            // Create a new logon session for the pet.
	        RegKey account = RegKey::HKCU.open(Constants::registryAccounts() +
                                               L"\\" + m.m_account_name);
	        std::wstring password = account.getValue(L"password");
			HANDLE token = INVALID_HANDLE_VALUE;
			if (!::LogonUser(m.m_account_name.c_str(), L".", password.c_str(),
				LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &token)) {
				m.m_logger.log(L"LogonUser()", GetLastError());
			}
			if (!::ImpersonateLoggedOnUser(token)) {
				m.m_logger.log(L"ImpersonateLoggedOnUser()", GetLastError());
			}
			auto_close<void*, &DestroyEnvironmentBlock> environment(NULL);
			if (!CreateEnvironmentBlock(&environment, token, FALSE)) {
				m.m_logger.log(L"CreateEnvironmentBlock()", GetLastError());
			}
			TCHAR profile_path[MAX_PATH];
			DWORD profile_path_size = sizeof profile_path / sizeof TCHAR;
			if (!::GetUserProfileDirectory(token, profile_path, &profile_path_size)) {
				m.m_logger.log(L"GetUserProfileDirectory()", GetLastError());
			}
			RevertToSelf();

			STARTUPINFO si = { sizeof(STARTUPINFO) };
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			PROCESS_INFORMATION pi = {};
			if (!::CreateProcessWithLogonW(m.m_account_name.c_str(), NULL, password.c_str(),
										   LOGON_WITH_PROFILE,
										   NULL, L"C:\\WINDOWS\\system32\\notepad.exe",
										   CREATE_UNICODE_ENVIRONMENT,
										   environment.get(), profile_path, &si, &pi)) {
				m.m_logger.log(L"CreateProcessWithLogonW()", GetLastError());
			} else {

				WaitForInputIdle(pi.hProcess, INFINITE);

				auto_privilege se_debug(SE_DEBUG_NAME);
				if (!::ImpersonateLoggedOnUser(token)) {
					m.m_logger.log(L"ImpersonateLoggedOnUser()", GetLastError());
				}
				if (!::OpenProcessToken(pi.hProcess,
					TOKEN_QUERY | TOKEN_DUPLICATE |
					TOKEN_ASSIGN_PRIMARY | TOKEN_IMPERSONATE, &m.m_session)) {
					m.m_logger.log(L"OpenProcessToken()", GetLastError());
				}
				RevertToSelf();

				CloseHandle(token);
				m.m_zombie = pi.hProcess;
				CloseHandle(pi.hThread);
			}

            // Start dispatching pet requests.
            m.m_dispatcher = makeMessageDispatcher(m.m_event_loop, m.m_request_dir);
            m.m_dispatcher->serve("echo", makeEchoHandler());
	        m.m_dispatcher->serve("GetClipboardData", makeGetClipboardDataHandler());
            m.m_dispatcher->serve("GetOpenFileNameA", makeGetOpenFileNameAHandler(&m));
            m.m_dispatcher->serve("GetSaveFileNameA", makeGetSaveFileNameAHandler(&m));
            m.m_dispatcher->serve("GetOpenFileNameW", makeGetOpenFileNameWHandler(&m));
            m.m_dispatcher->serve("GetSaveFileNameW", makeGetSaveFileNameWHandler(&m));
            m.m_dispatcher->serve("MAPISendMail", makeMAPISendMailHandler());
            m.m_dispatcher->serve("openurl", makeOpenPetURLHandler());
            m.m_dispatcher->serve("openstrict", makeOpenPetStrictHandler());

            // Setup the authenticating printers.
            if (m_user_password && wcscmp(m_user_password, L"") != 0) {
                m.m_scheduler->send(new UseServers(m, m_user_password));
            }
        }
    };

    class Delete : public Runnable
    {
        ActiveAccountImpl* m;

    public:
        Delete(ActiveAccountImpl* scope) : m(scope) {}
        virtual ~Delete() {}

        virtual void run() { delete m; }
    };

    /**
     * Setup only the state required to make an ActivePet handle for this account.
     */
    ActiveAccountImpl(std::wstring petname,
                      std::wstring account_name,
                      wchar_t* user_password) : m_petname(petname),
                                                m_account_name(account_name),
                                                m_job(NULL),
                                                m_event_loop(NULL),
                                                m_scheduler(NULL),
                                                m_logger(account_name),
												m_zombie(INVALID_HANDLE_VALUE),
                                                m_session(INVALID_HANDLE_VALUE),
                                                m_dispatcher(NULL),
                                                m_next_editable_name(0)
    {
        // Initialize the job.
        m_job = CreateJobObject(NULL, NULL);
	    if (NULL != m_job) {
            JOBOBJECT_BASIC_UI_RESTRICTIONS buir = { JOB_OBJECT_UILIMIT_HANDLES };
	        if (!SetInformationJobObject(m_job, JobObjectBasicUIRestrictions, &buir, sizeof buir)) {
                m_logger.log(L"SetInformationJobObject()", GetLastError());
	        }

            // Some apps fail to launch without access to the desktop window.
            // Doesn't seem to be necessary if Acrobat >= 7.0.7.
            // But, PowerPoint still wants it and so does PassPet.
	        if (!UserHandleGrantAccess(GetDesktopWindow(), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess()", GetLastError());
	        }

            // Give apps access to all the standard cursors.
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_ARROW), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_ARROW)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_IBEAM), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_IBEAM)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_WAIT), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_WAIT)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_CROSS), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_CROSS)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_UPARROW), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_UPARROW)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZE), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_SIZE)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_ICON), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_ICON)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZENWSE), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_SIZENWSE)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZENESW), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_SIZENESW)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZEWE), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_SIZEWE)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZENS), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_SIZENS)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_SIZEALL), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_SIZEALL)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_NO), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_NO)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_HAND), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_HAND)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_APPSTARTING), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_APPSTARTING)", GetLastError());
	        }
	        if (!UserHandleGrantAccess(LoadCursor(NULL, IDC_HELP), m_job, TRUE)) {
                m_logger.log(L"UserHandleGrantAccess(IDC_HELP)", GetLastError());
	        }
        } else {
            m_logger.log(L"CreateJobObject()", GetLastError());
        }

        // Setup the directories.
        m_request_dir = Constants::requestsPath(Constants::polarisDataPath(
                                    Constants::userProfilePath(m_account_name)));
        CreateDirectory(m_request_dir.c_str(), NULL);
        m_editable_dir = Constants::editablesPath(Constants::polarisDataPath(
                                    Constants::userProfilePath(m_account_name)));
        CreateDirectory(m_editable_dir.c_str(), NULL);
        
        // Start up the event thread.
        EventThread thread = makeEventThread(new Delete(this));
        m_event_loop = thread.loop;
        m_scheduler = makeTaskQueue(m_event_loop);
        m_scheduler->send(new Logon(*this, user_password));
        spawn(thread.spin);

        m_logger.log(L"Live...");
    }
	~ActiveAccountImpl() {
        delete m_dispatcher;
        delete m_scheduler;
    }
public:
    static ActivePet* make(std::wstring petname, wchar_t* user_password) {
        std::wstring petRegistryPath = Constants::registryPets() + L"\\" + petname;
	    RegKey petkey = RegKey::HKCU.open(petRegistryPath);
	    std::wstring account_name = petkey.getValue(L"accountName");
        if (account_name == L"") {
            return NULL;
        }
        ActiveAccountImpl* m = new ActiveAccountImpl(petname, account_name, user_password);

        // Get the account SID.
        DWORD sid_length = 0;
        DWORD domain_name_length = 0;
        SID_NAME_USE use;
        LookupAccountName(NULL, account_name.c_str(),
                          NULL, &sid_length,
                          NULL, &domain_name_length,
                          &use);
        BYTE* user_sid = new BYTE[sid_length];
        TCHAR* domain_name = new TCHAR[domain_name_length];
        if (!LookupAccountName(NULL, account_name.c_str(),
                               user_sid, &sid_length,
                               domain_name, &domain_name_length,
                               &use)) {
            m->m_logger.log(L"LookupAccountName()", GetLastError());
        }

        return new ActivePetImpl(petname, user_sid, m->m_job, m->m_scheduler, *m);
    }

    virtual std::wstring getPetname() { return m_petname; }

    virtual std::wstring mirror(std::wstring dir) {

        std::wstring pet_dir = m_editable_dir;

        // create a new folder under the editables folder to hold the editable file
        std::wstring::size_type i = 0;
        std::wstring::size_type j = dir.find(':');
        if (j != std::wstring::npos && i != j) {
            pet_dir += L"\\" + dir.substr(i, j - i);
            CreateDirectory(pet_dir.c_str(), NULL);
            i = ++j;
        } else {
            j = 0;
        }
        while (i != dir.length()) {
            j = dir.find('\\', i);
            if (j == std::wstring::npos) {
                pet_dir += L"\\" + dir.substr(i);
                CreateDirectory(pet_dir.c_str(), NULL);
                break;
            }
            if (i != j) {
                pet_dir += L"\\" + dir.substr(i, j - i);
                CreateDirectory(pet_dir.c_str(), NULL);
            }
            i = ++j;
        }

        return pet_dir;
    }

    virtual std::wstring invert(std::wstring path) {
        if (path.substr(0, m_editable_dir.length()) == m_editable_dir) {
            path = path.substr(m_editable_dir.length());
            if (path.find('\\') == 1) {
                path = path.substr(0, 1) + L":" + path.substr(1);
            }
        }
        return path;
    }

    virtual std::wstring editOld(std::wstring file, bool copy) {

        // See if the pet already has access to the file.
        if (copy) {
            auto_impersonation pet(m_session);
            auto_handle file_handle(CreateFile(file.c_str(), 
                                               FILE_EXECUTE | FILE_READ_DATA,
                                               FILE_SHARE_READ,
                                               NULL,
                                               OPEN_EXISTING,
                                               0,
                                               NULL));
            if (0 == GetLastError()) {
                return file;
            }
        }

        // TODO: This should be under the account key.
	    RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname);
        RegKey editables = petkey.open(Constants::editables());
        if (false && copy && petkey.getValue(Constants::isReadOnlyWithLinks()) == L"true") {
            // This code is incomprehensible.
            std::wstring editableParent = file.substr(0, file.rfind(L"\\"));
            FileACL acl(editableParent);
            acl.grant(m_account_name, FileACL::READ());
            RegKey bootActions = RegKey::HKCU.open(Constants::registryBootActions());
            RegKey nextKey = bootActions.create(strongPassword());
            nextKey.setValue(Constants::actionRevokeAccount(), editableParent);
            nextKey.setValue(Constants::accountName(), m_account_name);
            return file;
        }

        // Check if the pet already has a local copy of the original file.
	    for (int i = 0; true; ++i) {
		    RegKey live = editables.getSubKey(i);
		    if (!live.isGood()) {
			    break;
		    }
		    if (file == live.getValue(L"originalPath")) {
                std::wstring editable_file = live.getValue(L"editablePath");
				if (!copy) {
					// If we're about to save here, clear the way to avoid
					// triggering any overwrite confirmation prompts.
					DeleteFile(editable_file.c_str());
				}
			    return editable_file;
		    }
        }

        // Create an editable version.
        std::wstring editable_file = mirror(file.substr(0, file.rfind(L"\\"))) +
                                     file.substr(file.find_last_of('\\'));
        // if the file to be launched is "boxed" (has a .boxed suffix, used by polaris to
        // indicate that a safe launch should be made, in the icebox if the real file
        // extension inside the .boxed extension is not listed in the polaris registry key
        // list of suffixes), then strip the boxed extension before making the copy. This
        // way the editable file will have the right type to be interpreted correctly.
        std::wstring searchExtension = L"." + Constants::boxedFileExtension();
        size_t boxedIndex = editable_file.rfind(searchExtension); 
        if (boxedIndex != std::wstring::npos &&
            boxedIndex == editable_file.length() - searchExtension.length()) {
            editable_file = editable_file.substr(0,boxedIndex);
        }

		// Remember that we have this stray file lying around.
        while (true) {
            wchar_t buffer[16] = {};
            _itow(m_next_editable_name++, buffer, 10);
		    RegKey entry = editables.create(buffer);
		    if (entry.isGood()) {
		        entry.setValue(L"originalPath", file);
		        entry.setValue(L"editablePath", editable_file);
                break;
		    }
        }

		if (copy) {
			if (!CopyFile(file.c_str(), editable_file.c_str(), TRUE)) {
				m_logger.log(L"CopyFile()", GetLastError());
			} 
		} else {
			// If we're about to save here, clear the way to avoid
			// triggering any overwrite confirmation prompts.
			DeleteFile(editable_file.c_str());
		}

        // Build syncer command line.
        std::wstring exe = Constants::polarisExecutablesFolderPath() + L"\\powersync.exe";
        wchar_t commandLine[4096] = {};
        wcscat(commandLine, L"\"");
        wcscat(commandLine, exe.c_str());
        wcscat(commandLine, L"\" \"");
        wcscat(commandLine, editable_file.c_str());
        wcscat(commandLine, L"\" \"");
        wcscat(commandLine, file.c_str());
        wcscat(commandLine, L"\"");

        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi = {};
	    if (!CreateProcess(exe.c_str(),
                           commandLine,
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           NULL,
                           &si,
                           &pi)) {
            m_logger.log(L"CreateProcess()", GetLastError());
	    }
        BOOL r = AssignProcessToJobObject(m_job, pi.hProcess);
        if (!r) {
            m_logger.log(L"AssignProcessToJobObject()", GetLastError());
        }
        DWORD error = WaitForInputIdle(pi.hProcess, INFINITE);
        if (error != ERROR_SUCCESS) {
            m_logger.log(L"WaitForInputIdle()", GetLastError());
        }
        CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);

        return editable_file;
    }

	virtual std::wstring edit(std::wstring originalDocPath, bool copy) {

        // See if the pet already has access to the file.
        if (copy && petCanReadFile(m_petname, originalDocPath)) {
			return originalDocPath;
        }

        // ??? TODO: This should be under the account key.
	    RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname);
        RegKey editables = petkey.open(Constants::editables());
        if (copy && petkey.getValue(Constants::isReadOnlyWithLinks()) == L"true") {
			std::wstring accountName = petkey.getValue(Constants::accountName());
			grantReadWithRevoke(accountName, originalDocPath);
			// This is done in 2 steps because early experiments said that the
			// acl-setting for a large folder would take a long time, and the plan was to make the
			// setting of the folder asynchronous, guaranteeing only the file itself. However,
			// recent experiments suggest that the acl setting of the folder is quite fast.
			// Left code as is, in case we have to make it asynch again.
			if (petCanReadFile(m_petname, originalDocPath)) { 
				size_t slashLoc = originalDocPath.rfind(L"\\");
				if (slashLoc != std::wstring::npos) {
					std::wstring editableParent = originalDocPath.substr(0, slashLoc);
					grantReadWithRevoke(accountName, editableParent);		
				}
				return originalDocPath;
			} 
        }

        // Check if the pet already has a local copy of the original file.
	    for (int i = 0; true; ++i) {
		    RegKey live = editables.getSubKey(i);
		    if (!live.isGood()) {
			    break;
		    }
		    if (originalDocPath == live.getValue(L"originalPath")) {
                std::wstring editable_file = live.getValue(L"editablePath");
				if (!copy) {
					// If we're about to save here, clear the way to avoid
					// triggering any overwrite confirmation prompts.
					DeleteFile(editable_file.c_str());
				}
			    return editable_file;
		    }
        }

        // Create an editable version.
        std::wstring editable_file = mirror(originalDocPath.substr(0, originalDocPath.rfind(L"\\"))) +
                                     originalDocPath.substr(originalDocPath.find_last_of('\\'));
        // if the file to be launched is "boxed" (has a .boxed suffix, used by polaris to
        // indicate that a safe launch should be made, in the icebox if the real file
        // extension inside the .boxed extension is not listed in the polaris registry key
        // list of suffixes), then strip the boxed extension before making the copy. This
        // way the editable file will have the right type to be interpreted correctly.
        std::wstring searchExtension = L"." + Constants::boxedFileExtension();
        size_t boxedIndex = editable_file.rfind(searchExtension); 
        if (boxedIndex != std::wstring::npos &&
            boxedIndex == editable_file.length() - searchExtension.length()) {
            editable_file = editable_file.substr(0,boxedIndex);
        }

		// Remember that we have this stray file lying around.
        while (true) {
            wchar_t buffer[16] = {};
            _itow(m_next_editable_name++, buffer, 10);
		    RegKey entry = editables.create(buffer);
		    if (entry.isGood()) {
		        entry.setValue(L"originalPath", originalDocPath);
		        entry.setValue(L"editablePath", editable_file);
                break;
		    }
        }
		
		// if readonlywith links but the acl setting did not work,
		// we not only make the copy, but here we will
		// also copy the "_files" folder associated with the file (if there
		// is one)
		if (copy && petkey.getValue(Constants::isReadOnlyWithLinks()) == L"true") {
			std::wstring filesExtension = L"_files";
			std::wstring originalFilesFolder = originalDocPath.substr(0, originalDocPath.rfind(L".")) + filesExtension;
			std::wstring editableFilesFolder = editable_file.substr(0, editable_file.rfind(L".")) + filesExtension;
			int err = _wmkdir(editableFilesFolder.c_str());
			copyDirectory(originalFilesFolder, editableFilesFolder);
			// arrange for deletion of the folder at next reboot
			RegKey actionsKey = RegKey::HKCU.open(Constants::registryBootActions());
			RegKey deleteKey = actionsKey.create(strongPassword());
			deleteKey.setValue(Constants::actionDeleteDir(), editableFilesFolder);
		}				

		if (copy) {
			if (!CopyFile(originalDocPath.c_str(), editable_file.c_str(), TRUE)) {
				m_logger.log(L"CopyFile()", GetLastError());
			} 
		} else {
			// If we're about to save here, clear the way to avoid
			// triggering any overwrite confirmation prompts.
			DeleteFile(editable_file.c_str());
		}

        // Build syncer command line.
        std::wstring exe = Constants::polarisExecutablesFolderPath() + L"\\powersync.exe";
        wchar_t commandLine[4096] = {};
        wcscat(commandLine, L"\"");
        wcscat(commandLine, exe.c_str());
        wcscat(commandLine, L"\" \"");
        wcscat(commandLine, editable_file.c_str());
        wcscat(commandLine, L"\" \"");
        wcscat(commandLine, originalDocPath.c_str());
        wcscat(commandLine, L"\"");

        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi = {};
	    if (!CreateProcess(exe.c_str(),
                           commandLine,
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           NULL,
                           &si,
                           &pi)) {
            m_logger.log(L"CreateProcess()", GetLastError());
	    }
        BOOL r = AssignProcessToJobObject(m_job, pi.hProcess);
        if (!r) {
            m_logger.log(L"AssignProcessToJobObject()", GetLastError());
        }
        DWORD error = WaitForInputIdle(pi.hProcess, INFINITE);
        if (error != ERROR_SUCCESS) {
            m_logger.log(L"WaitForInputIdle()", GetLastError());
        }
        CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);

        return editable_file;
    }

	virtual DWORD launch(std::wstring exe, std::wstring args) {
		// Build the command line.
	    wchar_t commandLine[4096] = { '\0' };
        if (args != L"") {
		    wcscat(commandLine, L"\"");
		    wcscat(commandLine, exe.c_str());
		    wcscat(commandLine, L"\" ");
		    wcscat(commandLine, args.c_str());
        }

		if (!::ImpersonateLoggedOnUser(m_session)) {
			m_logger.log(L"ImpersonateLoggedOnUser()", GetLastError());
		}
        auto_close<void*, &DestroyEnvironmentBlock> environment(NULL);
	    if (!CreateEnvironmentBlock(&environment, m_session, FALSE)) {
            m_logger.log(L"CreateEnvironmentBlock()", GetLastError());
	    }
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi = {};
	    if (!CreateProcessAsUser(m_session,
							     exe.c_str(),
							     commandLine,
							     NULL,
							     NULL,
							     FALSE,
							     CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE,
							     environment.get(),
							     NULL, // lpCurrentDirectory,
							     &si,
							     &pi)) {
            m_logger.log(L"CreateProcessAsUser()", GetLastError());
	    }
        CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);
		RevertToSelf();
        return pi.dwProcessId;
    }

private:
    void recursive_delete(std::wstring dir) {
        WIN32_FIND_DATA file_data;
        HANDLE find = FindFirstFile((dir + L"\\*").c_str(), &file_data);
        if (find != INVALID_HANDLE_VALUE) {
            do {
                std::wstring filename = file_data.cFileName;
                std::wstring filename_abs = dir + L"\\" + filename;
                if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (filename != L"." && filename != L"..") {
                        recursive_delete(filename_abs);
                        RemoveDirectory(filename_abs.c_str());
                    }
                } else {
	                RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname);
                    RegKey editables = petkey.open(Constants::editables());
	                for (int i = 0; true; ++i) {
		                RegKey live = editables.getSubKey(i);
		                if (!live.isGood()) {
			                break;
		                }
		                if (filename_abs == live.getValue(L"editablePath")) {
                            // Overwrite the original if the pet version is newer.
                            WIN32_FILE_ATTRIBUTE_DATA editable_attrs = {};
		                    BOOL editable_good = GetFileAttributesEx(filename_abs.c_str(),
                                                                     GetFileExInfoStandard,
                                                                     &editable_attrs);
                            std::wstring original_path = live.getValue(L"originalPath");
                            WIN32_FILE_ATTRIBUTE_DATA original_attrs = {};
		                    BOOL original_good = GetFileAttributesEx(original_path.c_str(),
                                                                     GetFileExInfoStandard,
                                                                     &original_attrs);
                            if (editable_good && original_good &&
                                CompareFileTime(&editable_attrs.ftLastWriteTime,
                                                &original_attrs.ftLastWriteTime) > 0) {
                                if (!CopyFile(filename_abs.c_str(), original_path.c_str(), FALSE)) {
                                    m_logger.log(L"CopyFile(\"" + filename_abs + L"\", \"" +
                                                 original_path + L"\")", GetLastError());
                                }
                            }
		                }
                    }
                    DeleteFile(filename_abs.c_str());
                }
            } while (FindNextFile(find, &file_data));
            FindClose(find);
        }
    }

public:

    virtual void kill() {

        // Stop processing messages.
        delete m_dispatcher;
        m_dispatcher = NULL;
        m_scheduler->close();

        // Logoff.
        if (!TerminateJobObject(m_job, -1)) {
            m_logger.log(L"TerminateJobObject()", GetLastError());
        }
        CloseHandle(m_job);
        m_job = NULL;
		if (INVALID_HANDLE_VALUE != m_zombie) {
			if (!TerminateProcess(m_zombie, 0)) {
				m_logger.log(L"TerminateProcess()", GetLastError());
			}
			CloseHandle(m_zombie);
			m_zombie = INVALID_HANDLE_VALUE;
		}
        CloseHandle(m_session);
        m_session = INVALID_HANDLE_VALUE;

        // Cleanse the requests directory.
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((m_request_dir + L"\\*").c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE) {
            while (true) {
                // only deleting request and response files. if there is anything else,
                // leave it so we can see it when debugging, since only request and response
                // files should be there.
                std::wstring filename = findFileData.cFileName;
                if (filename.find(L".response") != std::wstring.npos || 
                    filename.find(L".request") != std::wstring.npos) {
                    DeleteFile((m_request_dir + L"\\" + filename).c_str());
                }
                if (FindNextFile(hFind,&findFileData) == 0) {break;}
            }
            FindClose(hFind);
        }

        // Delete editables, carefully.
        recursive_delete(m_editable_dir);

        // Clean the registry.
	    RegKey petkey = RegKey::HKCU.open(Constants::registryPets() + L"\\" + m_petname);
        RegKey editables = petkey.open(Constants::editables());
        editables.removeSubKeys();

        m_logger.log(L"Dead.");
    }
};

ActivePet* openPet(std::wstring petname, wchar_t* user_password) {
    return ActiveAccountImpl::make(petname, user_password);
}
