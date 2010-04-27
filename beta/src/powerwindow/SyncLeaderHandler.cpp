#include "StdAfx.h"
#include "SyncLeaderHandler.h"

#include "../polaris/auto_handle.h"
#include "../polaris/Runnable.h"
#include "../polaris/Logger.h"

/*
 * Cannot just copyfile because the copyfile operation locks the file open too harshly and too long:
 * if the user is running an app like Word that saves by doing a temp save, delete original, rename temp,
 * and if the user does a series of such saves at high speed, one of the copyfiles will interfere with
 * one of the Word saves, then Word will conclude the file is read-only forever, and the user will
 * be faced with the puzzling requirement to SaveAs to a new name.
 *
 * TODO: Warning. this uses unnatural returns from deep if clauses. Rewrite.
 */
bool copyFileDeterminedly(std::wstring fromFile, std::wstring toFile) {
    for (int i = 0; i < 10; ++i) {
		HANDLE theFile = CreateFile(fromFile.c_str(), GENERIC_READ, 
			FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (theFile != INVALID_HANDLE_VALUE) {
			DWORD fileSize = GetFileSize(theFile, NULL);
			DWORD const bufferMaxSize = 4000000;
			if (fileSize == INVALID_FILE_SIZE || fileSize > bufferMaxSize) {
				CloseHandle(theFile);
				if (CopyFile(fromFile.c_str(),toFile.c_str(),FALSE)) {return true;}
			} else {
				BYTE *buffer = (BYTE*)malloc(fileSize);
				if (buffer == NULL) {
					CloseHandle(theFile);
					if (CopyFile(fromFile.c_str(),toFile.c_str(),FALSE)) {return true;}
				} else {
					DWORD bytesReadCount = 0;
					ReadFile(theFile, buffer, fileSize, &bytesReadCount, NULL); 
					CloseHandle(theFile);
					if (bytesReadCount == fileSize) {
						HANDLE outFile = CreateFile(toFile.c_str(), GENERIC_WRITE, 
							NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (outFile != INVALID_HANDLE_VALUE) {
							DWORD bytesWrittenCount = 0;
							WriteFile(outFile, buffer, fileSize, &bytesWrittenCount, NULL);
							CloseHandle(outFile);
							free(buffer);
							if (bytesWrittenCount == fileSize) {return true;}
						} else {
							free(buffer);
							if (CopyFile(fromFile.c_str(),toFile.c_str(),FALSE)) {return true;}
						}
					} else {
						free(buffer);
						if (CopyFile(fromFile.c_str(),toFile.c_str(),FALSE)) {return true;}
					}
				}
			}
		}
        Logger::basicLog(L"hiccup copying file");
        Sleep(100);
    }
    Logger::basicLog(L"copyFileDeterminedly could not copy file " + fromFile + L" to " + toFile);
    return false;
}

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
            m_logger.log(L"GetFileAttributesEX() failed getting leader_attrs", GetLastError() );
			return;
		}
		if (CompareFileTime(&leader_attrs.ftLastWriteTime, &m_lastModified) != 0) {
            // the leader has changed, we are going to do an update
            // if the follower is locked, unlock
            if (m_followerLockHandle != INVALID_HANDLE_VALUE) {CloseHandle(m_followerLockHandle);}
            WIN32_FILE_ATTRIBUTE_DATA follower_attrs = {};
			if (!GetFileAttributesEx(m_followerPath.c_str(), GetFileExInfoStandard, &follower_attrs)) {
				printf("GetFileAttributesEx() failed: %d\n", GetLastError());
                m_logger.log(L"GetFileAttributesEX() failed getting follower_attrs", GetLastError() );
			}
			if (CompareFileTime(&leader_attrs.ftLastWriteTime, &follower_attrs.ftLastWriteTime) != 0) {
				if (!copyFileDeterminedly(m_leaderPath,m_followerPath)) {
					printf("CopyFile() failed: %d\n", GetLastError());
                    m_logger.log(L"CopyFile() failed from leader to follower", GetLastError() );
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
					        m_followerLockHandle = CreateFile(m_followerPath.c_str(), FILE_READ_DATA, FILE_SHARE_READ,
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
