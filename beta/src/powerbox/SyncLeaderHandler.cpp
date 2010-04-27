#include "StdAfx.h"
#include "syncleaderhandler.h"
#include "../polaris/auto_handle.h"
#include "../polaris/Runnable.h"
#include "../polaris/Logger.h"

class SyncLeaderHandler : public Runnable
{
	std::wstring m_leaderPath;
	std::wstring m_followerPath;
    Logger m_logger;
    HANDLE m_followerLockHandle;
	FILETIME m_lastModified;

public:
	SyncLeaderHandler(std::wstring leaderPath,
					  std::wstring followerPath) : m_leaderPath(leaderPath),
												   m_followerPath(followerPath),
                                                   m_logger(L"SyncLeaderHandler leader " + leaderPath + L" follower " + followerPath)
	{
        m_followerLockHandle = INVALID_HANDLE_VALUE;
        WIN32_FILE_ATTRIBUTE_DATA leader_attrs = {};
		if (!GetFileAttributesEx(m_leaderPath.c_str(), GetFileExInfoStandard, &leader_attrs)) {
			ZeroMemory(&m_lastModified, sizeof(m_lastModified));
		} else {
			m_lastModified = leader_attrs.ftLastWriteTime;
		}
	}
	virtual ~SyncLeaderHandler() {
        if (m_followerLockHandle != INVALID_HANDLE_VALUE) {CloseHandle(m_followerLockHandle);}
    }
	virtual void run()
	{
        WIN32_FILE_ATTRIBUTE_DATA leader_attrs = {};
		if (!GetFileAttributesEx(m_leaderPath.c_str(), GetFileExInfoStandard, &leader_attrs)) {
			printf("GetFileAttributesEx() failed: %d\n", GetLastError());
            m_logger.log(L"GetFileAttributesEX() failed", GetLastError() );
			return;
		}
		if (CompareFileTime(&leader_attrs.ftLastWriteTime, &m_lastModified) != 0) {
            // the leader has changed, we are going to do an update
            // if the follower is locked, unlock
            if (m_followerLockHandle != INVALID_HANDLE_VALUE) {CloseHandle(m_followerLockHandle);}
            WIN32_FILE_ATTRIBUTE_DATA follower_attrs = {};
			if (!GetFileAttributesEx(m_followerPath.c_str(), GetFileExInfoStandard, &follower_attrs)) {
				printf("GetFileAttributesEx() failed: %d\n", GetLastError());
                m_logger.log(L"GetFileAttributesEX() failed", GetLastError() );
			}
			if (CompareFileTime(&leader_attrs.ftLastWriteTime, &follower_attrs.ftLastWriteTime) != 0) {
				if (!CopyFile(m_leaderPath.c_str(),m_followerPath.c_str(),FALSE)) {
					printf("CopyFile() failed: %d\n", GetLastError());
                    m_logger.log(L"CopyFile() failed", GetLastError() );
				} else {
					// Mirror the write time.
					m_followerLockHandle = CreateFile(m_followerPath.c_str(), FILE_WRITE_ATTRIBUTES, 0,
                                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (INVALID_HANDLE_VALUE != m_followerLockHandle) {
						if (!SetFileTime(m_followerLockHandle, NULL, NULL, &leader_attrs.ftLastWriteTime)) {
							printf("SetFileTime() failed: %d\n", GetLastError());
                            m_logger.log(L"SetFileTime() failed", GetLastError() );
                        }
                        //regardless of if the file time set worked, close the attribute-writing handle
                        CloseHandle(m_followerLockHandle);
                        //if the leader is locked, lock the follower also
                        HANDLE leaderLock = CreateFile(m_leaderPath.c_str(), FILE_WRITE_DATA, 0,
                                                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (INVALID_HANDLE_VALUE != leaderLock) {
                            CloseHandle(leaderLock);
                            m_followerLockHandle = INVALID_HANDLE_VALUE;
                        } else {
                            //leader is locked, do same to follower
					        m_followerLockHandle = CreateFile(m_followerPath.c_str(), FILE_WRITE_DATA, 0,
                                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        }                        
					} else {
						printf("CreateFile() failed: %d\n", GetLastError());
                        m_logger.log(L"CreateFile() failed", GetLastError() );
					}
				}
			}
		}
		m_lastModified = leader_attrs.ftLastWriteTime;
	}
};

Runnable* makeSyncLeaderHandler(std::wstring leaderPath, std::wstring followerPath)
{
	return new SyncLeaderHandler(leaderPath, followerPath);
}
