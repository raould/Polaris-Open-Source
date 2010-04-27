// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "StdAfx.h"
#include "SyncLeaderHandler.h"

#include "../polaris/auto_handle.h"
#include "../polaris/Runnable.h"
#include "../polaris/Logger.h"
//#include <Windows.h>

/*
 * Cannot just copyfile because the copyfile operation locks the file open too harshly and too long:
 * if the user is running an app like Word that saves by doing a temp save, delete original, rename temp,
 * and if the user does a series of such saves at high speed, one of the copyfiles will interfere with
 * one of the Word saves, then Word will conclude the file is read-only forever, and the user will
 * be faced with the puzzling requirement to SaveAs to a new name.
 *
 */
bool copyFileDeterminedly(std::wstring fromFile, std::wstring toFile) {
	Logger copyLog(L"copyFileDeterminedly");
	copyLog.log(L"starting copy " + toFile);
	int i = 0;
	bool copySucceeded = false;
	while (i < 10 && !copySucceeded) {
		++i;
		// TODO here in the comments is an incomplete attempt to make a useful high performance
		// shortcut to the copying process by simply creating a hard link.
		// Will only work with files on the same ntfs volume. Needs
		// to be undertaken at a higher level, to avoid funny effects like
		// having the syncer attempt to lock the follower if the leader is 
		// locked: if a hard link is used, the lock state automatically follows
		// along, a separate attempt by the syncer to lock the follower will
		// at best cause no harm. documentation of hard link also suggests poosible
		// trouble with timely updating of the file attributes, would need
		// experimentation.
		//copySucceeded = CreateHardLink(toFile.c_str(), fromFile.c_str(), NULL);
		//if (copySucceeded) {break;}
		//copyLog.log(L"hard link failed", GetLastError());
		HANDLE theFile = CreateFile(fromFile.c_str(), GENERIC_READ, 
			FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ, 
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (theFile != INVALID_HANDLE_VALUE) {
			DWORD fileSize = GetFileSize(theFile, NULL);
			// if file is too large, virtual memory will thrash, do the regular copyfile anyway
			if (fileSize > 50000000) {
				CloseHandle(theFile);
				copySucceeded = CopyFile(fromFile.c_str(),toFile.c_str(),FALSE);
			} else if (fileSize == INVALID_FILE_SIZE) {
				CloseHandle(theFile);
				Logger::basicLog(L"invalid file size copying determinedly");
			} else {
				BYTE *buffer = (BYTE*)malloc(fileSize);
				if (buffer == NULL) {
					CloseHandle(theFile);
					Logger::basicLog(L"buffer null in copyfiledeterminedly");
				} else {
					DWORD bytesReadCount = 0;
					ReadFile(theFile, buffer, fileSize, &bytesReadCount, NULL); 
					CloseHandle(theFile);
					if (bytesReadCount == fileSize) {
						HANDLE outFile = CreateFile(toFile.c_str(), FILE_WRITE_DATA, 
							NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						if (outFile != INVALID_HANDLE_VALUE) {
							DWORD bytesWrittenCount = 0;
							WriteFile(outFile, buffer, fileSize, &bytesWrittenCount, NULL);
							CloseHandle(outFile);
							free(buffer);
							copySucceeded = bytesWrittenCount == fileSize;
							if (!copySucceeded) {Logger::basicLog(L"error in copy determinedly: bytes written != filesize");}
						} else {
							free(buffer);
							Logger::basicLog(L"outfile invalid copying determinedly");
						}
					} else {
						free(buffer);
						Logger::basicLog(L"invalid file size copying determinedly");
					}
				}
			}
		}
        Logger::basicLog(L"hiccup copying file");
        Sleep(100);
    }
	if (!copySucceeded) {
		copySucceeded = CopyFile(fromFile.c_str(),toFile.c_str(),FALSE);
	}
	if (!copySucceeded) {
		Logger::basicLog(L"copyFileDeterminedly could not copy file " + fromFile + L" to " + toFile);
	}
    return copySucceeded;
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
		m_logger.log(L"running");
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
					MessageBox(NULL, (L"Could not save:\n " + 
						m_followerPath + 
						L"\n\nPerhaps the file being edited is Read-Only. \n\nPlease use SaveAs to save the file someplace else").c_str(), 
						L"Polaris Synchronization Failure", MB_OK);
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
						m_followerLockHandle = INVALID_HANDLE_VALUE;
                        //if the leader is locked, lock the follower also
                        HANDLE leaderLock = CreateFile(m_leaderPath.c_str(), FILE_WRITE_DATA, 0,
                                                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (INVALID_HANDLE_VALUE != leaderLock) {
                            CloseHandle(leaderLock);
                        } else {
                            //leader is locked, do same to follower
							// TODO: figure out what to do about locking the follower if the leader is locked. The problem
							//  is that if we lock the follower, the lock holds, not until the app is done with the doc,
							//  but until the app itself is shut down. So the follower (i.e., original) stays locked 
							//  open, it cannot be moved or deleted or renamed until the app is shutdown.
					        //m_followerLockHandle = CreateFile(m_followerPath.c_str(), FILE_READ_DATA, FILE_SHARE_READ,
                            //                          NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
