// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "ProcessWatcher.h"

#include "ActivePet.h"
#include "ActiveWorld.h"
#include "EventLoop.h"

#include "../polaris/auto_array.h"
#include "../polaris/auto_handle.h"
#include "../polaris/auto_privilege.h"
#include "../polaris/Constants.h"
#include "../polaris/Logger.h"
#include "../polaris/Runnable.h"
#include "../poladvr/poladvr.h"

#include <winioctl.h>
#include <sddl.h>
#include <AccCtrl.h>
#include <Aclapi.h>

class ProcessDeath : public Runnable
{
    EventLoop* m_event_loop;
    ActiveWorld* m_world;
    HANDLE m_process;
    ActivePet* m_pet;

public:
    ProcessDeath(EventLoop* event_loop,
                 ActiveWorld* world,
                 HANDLE process,
                 ActivePet* pet) : m_event_loop(event_loop),
                                   m_world(world),
                                   m_process(process),
                                   m_pet(pet) {}
    virtual ~ProcessDeath() {
        CloseHandle(m_process);
    }

    virtual void run() {
        // See if the pet is still alive.
        std::list<ActivePet*>& pets = m_world->list();
        std::list<ActivePet*>::iterator i = std::find(pets.begin(), pets.end(), m_pet);
        if (pets.end() != i) {
            if ((*i)->release(m_process)) {
                // The last process has terminated, kill the active pet.
                (*i)->send(new ActiveAccount::Kill());
                pets.erase(i);
            }
        }

        // Done waiting for the process.
        m_event_loop->ignore(m_process, this);
        delete this;
    }
};

class ProcessWatcher : public Runnable
{
    EventLoop* m_event_loop;
    ActiveWorld* m_world;
    Logger m_logger;

    SC_HANDLE m_service;
    HANDLE m_channel;
	HANDLE m_event;

public:
    ProcessWatcher(EventLoop* event_loop,
                   ActiveWorld* world) : m_event_loop(event_loop),
                                         m_world(world), 
                                         m_logger(L"ProcessWatcher"),
                                         m_service(NULL),
                                         m_channel(INVALID_HANDLE_VALUE),
                                         m_event(NULL)
    {
        const wchar_t* driver_name = L"poladvr";

        auto_close<SC_HANDLE, &::CloseServiceHandle> scm(
            ::OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE));
        if (scm.get()) {
            // Install the process creation watcher.
            std::wstring path = Constants::polarisExecutablesFolderPath() +
                                L"\\" + driver_name + L".sys";
            // std::wstring path = L"C:\\poladvr\\objchk_wxp_x86\\i386\\poladvr.sys";
            DWORD access_rights = SERVICE_START | DELETE | SERVICE_STOP;
            m_service = ::CreateService(scm.get(), driver_name, driver_name, access_rights,
                                        SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                                        SERVICE_ERROR_SEVERE, path.c_str(),
                                        NULL, NULL, NULL, NULL, NULL);
            if (!m_service && ERROR_SERVICE_EXISTS == ::GetLastError()) {
                m_service = ::OpenService(scm.get(), driver_name, access_rights);
            }
            if (m_service) {
                if (!::StartService(m_service, 0, NULL)) {
                    DWORD error = ::GetLastError();
                    switch (error) {
                    case ERROR_SERVICE_ALREADY_RUNNING:
                        break;
                    default:
                        m_logger.log(L"Failed to start service", error);
                    }
                }

                // Create the process creation event.
                m_channel = ::CreateFile(L"\\\\.\\POLADVR", GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                         OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE != m_channel) {

                    // Create the new process notification event.
                    m_event = ::CreateEvent(NULL, TRUE, FALSE, NULL);
                    if (NULL != m_event) {
                        m_event_loop->watch(m_event, this);
                        PoladvrSetupArgs setup_args = { m_event };
                        if (LookupPrivilegeValue(NULL, TEXT("SeImpersonatePrivilege"),
                                                 &setup_args.non_pet_privilege)) {
                            m_logger.log(L"Non-pet privilege low is:",
                                         setup_args.non_pet_privilege.LowPart);
                            m_logger.log(L"Non-pet privilege high is:",
                                         setup_args.non_pet_privilege.HighPart);
                            DWORD received = 0;
                            if (!DeviceIoControl(m_channel, IOCTL_POLADVR_SETUP,
                                                 &setup_args, sizeof setup_args,
                                                 NULL, 0, &received, NULL)) {
                                m_logger.log(L"Driver setup failed", ::GetLastError());
                            }
                        } else {
                            m_logger.log(L"LookupPrivilegeValue(SeImpersonatePrivilege)",
                                         ::GetLastError());
                        }
                    } else {
                        m_logger.log(L"Failed to create event", ::GetLastError());
                    }
                } else {
                    m_logger.log(L"Failed to connect to driver", ::GetLastError());
                }
            } else {
                m_logger.log(L"Failed to create service", ::GetLastError());
            }
        } else {
            m_logger.log(L"Failed to open SCM", GetLastError());
        }
    }

    virtual ~ProcessWatcher() {

        ::CloseHandle(m_channel);

        SERVICE_STATUS ss;
        ::ControlService(m_service, SERVICE_CONTROL_STOP, &ss);
        ::DeleteService(m_service);
        ::CloseServiceHandle(m_service);

        m_event_loop->ignore(m_event, this);
        ::CloseHandle(m_event);
    }

    virtual void run() {
        DWORD process_id = 0;
        DWORD received = 0;
        if (::DeviceIoControl(m_channel, IOCTL_POLADVR_CHECK,
                              NULL, 0, &process_id, sizeof process_id, &received, NULL)) {
            m_logger.log(L"Got process creation event", process_id);

            // Put the process in the corresponding job.
            DWORD decision;     // What to do with the process?
            auto_privilege se_debug(SE_DEBUG_NAME);
            auto_handle process(::OpenProcess(PROCESS_ALL_ACCESS, FALSE, process_id));
            if (process.get()) {
                PSID user_sid = NULL;
                PSECURITY_DESCRIPTOR user_sec = NULL;
                DWORD error = ::GetSecurityInfo(process.get(), SE_KERNEL_OBJECT,
                                                OWNER_SECURITY_INFORMATION, &user_sid,
                                                NULL, NULL, NULL, &user_sec);
                if (ERROR_SUCCESS == error) {
                    ActivePet* pet = m_world->findByUser(user_sid);
                    if (pet) {
                        if (pet->manage(process.get())) {
                            decision = IOCTL_POLADVR_RELEASE;

                            // Keep track of the process.
                            HANDLE death = INVALID_HANDLE_VALUE;
                            if (::DuplicateHandle(::GetCurrentProcess(), process.get(),
                                                  ::GetCurrentProcess(), &death,
                                                  0, FALSE, DUPLICATE_SAME_ACCESS)) {
                                m_event_loop->watch(death,
                                    new ProcessDeath(m_event_loop, m_world, death, pet));
                            } else {
                                m_logger.log(L"DuplicateHandle()", GetLastError());
                            }

                        } else {
                            decision = IOCTL_POLADVR_CONTINUE;
                        }
                    } else {
                        decision = IOCTL_POLADVR_RELEASE;
                    }
                    LocalFree(user_sec);
                } else {
                    decision = IOCTL_POLADVR_CONTINUE;
                    m_logger.log(L"GetSecurityInfo()", error);
                }
/* Should be using OpenProcessToken instead of GetSecurityInfo
                auto_handle token(NULL);
                if (::OpenProcessToken(process.get(), TOKEN_QUERY, &token)) {
                    DWORD length = 0;
                    ::GetTokenInformation(token.get(), TokenUser, NULL, 0, &length);
                    BYTE* buffer = new BYTE[length];
                    if (::GetTokenInformation(token.get(), TokenUser, buffer, length, &length)) {
                        ActivePet* pet = m_world->findByUser(((TOKEN_USER*)buffer)->User.Sid);
                        if (pet) {
                            m_logger.log(L"found corresponding pet!");
                            pet->manage(process.get());
                            if (::DeviceIoControl(m_channel, IOCTL_POLADVR_RELEASE,
                                                  &process_id, sizeof process_id,
                                                  NULL, 0, &received, NULL)) {
                                m_logger.log(L"Released process", process_id);
                                released = true;
                            } else {
                                m_logger.log(L"Driver release failed", ::GetLastError());
                            }
                        }
                    } else {
                        m_logger.log(L"GetTokenInformation()", ::GetLastError());
                    }
                    delete[] buffer;
                } else {
                    m_logger.log(L"OpenProcessToken()", ::GetLastError());
                }
*/
            } else {
                decision = IOCTL_POLADVR_CONTINUE;
                m_logger.log(L"OpenProcess()", ::GetLastError());
            }
            if (::DeviceIoControl(m_channel, decision, &process_id, sizeof process_id,
                                  NULL, 0, &received, NULL)) {
                switch (decision) {
                case IOCTL_POLADVR_CONTINUE:
                    m_logger.log(L"Continued process", process_id);
                    break;
                case IOCTL_POLADVR_RELEASE:
                    m_logger.log(L"Released process", process_id);
                    break;
                default:
                    m_logger.log(L"Undocumented decision", process_id);
                }
            } else {
                m_logger.log(L"Driver communication failed", ::GetLastError());
            }
        } else {
             m_logger.log(L"Driver check failed", ::GetLastError());
        }
    }
};

Runnable* MakeProcessWatcher(EventLoop* event_loop, ActiveWorld* world) {
    return new ProcessWatcher(event_loop, world);
}
