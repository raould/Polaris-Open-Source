#include "StdAfx.h"
#include "syncfollowerhandler.h"
#include "../polaris/auto_handle.h"
#include "../polaris/Runnable.h"
#include "../polaris/Logger.h"


// The SyncFollowerHandler is in charge of updating the editable (leader) if the 
// original (follower) is updated by an outside program (outside the pet that is
// editing the editable). See the Syncer class doc for discussion of all the 
// cases that must be handled successfully for leader/follower syncing
class SyncFollowerHandler : public Runnable
{
	std::wstring m_leaderPath;
	std::wstring m_followerPath;
    Logger m_logger;

	FILETIME m_lastModified;

public:
	SyncFollowerHandler(std::wstring leaderPath,
					    std::wstring followerPath) : m_leaderPath(leaderPath),
												     m_followerPath(followerPath),
                                                     m_logger(L"SyncFollowerHandler leader " + leaderPath + L" follower " + followerPath)
	{
        WIN32_FILE_ATTRIBUTE_DATA follower_attrs = {};
		if (!GetFileAttributesEx(m_followerPath.c_str(), GetFileExInfoStandard, &follower_attrs)) {
			ZeroMemory(&m_lastModified, sizeof(m_lastModified));
		} else {
			m_lastModified = follower_attrs.ftLastWriteTime;
		}
	}
	virtual ~SyncFollowerHandler() {}
	virtual void run()
	{
        WIN32_FILE_ATTRIBUTE_DATA follower_attrs = {};
		if (!GetFileAttributesEx(m_followerPath.c_str(), GetFileExInfoStandard, &follower_attrs)) {
			printf("GetFileAttributesEx() failed: %d\n", GetLastError());
            m_logger.log(L"GetFileAttributesEx() failed", GetLastError() );
			return;
		}
		if (CompareFileTime(&follower_attrs.ftLastWriteTime, &m_lastModified) != 0) {
            WIN32_FILE_ATTRIBUTE_DATA leader_attrs = {};
			if (!GetFileAttributesEx(m_leaderPath.c_str(), GetFileExInfoStandard, &leader_attrs)) {
				printf("GetFileAttributesEx() failed: %d\n", GetLastError());
                m_logger.log(L"GetFileAttributesEx() failed", GetLastError() );
			}
			if (CompareFileTime(&leader_attrs.ftLastWriteTime, &follower_attrs.ftLastWriteTime) != 0) {
				if (!CopyFile(m_followerPath.c_str(),m_leaderPath.c_str(),FALSE)) {
					printf("CopyFile() failed: %d\n", GetLastError());
                    m_logger.log(L"CopyFile() failed", GetLastError() );
				} else {
					// Mirror the write time.
					auto_handle leader(CreateFile(m_leaderPath.c_str(), FILE_WRITE_ATTRIBUTES, 0,
                                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
					if (INVALID_HANDLE_VALUE != leader.get()) {
						if (!SetFileTime(leader.get(), NULL, NULL, &follower_attrs.ftLastWriteTime)) {
							printf("SetFileTime() failed: %d\n", GetLastError());
                            m_logger.log(L"SetFileTime() failed", GetLastError());
						}
					} else {
						printf("CreateFile() failed: %d\n", GetLastError());
                        m_logger.log(L"CreateFile() failed", GetLastError() );
					}
				}
			}
		}
		m_lastModified = follower_attrs.ftLastWriteTime;
	}
};

Runnable* makeSyncFollowerHandler(std::wstring leaderPath, std::wstring followerPath)
{
	return new SyncFollowerHandler(leaderPath, followerPath);
}
