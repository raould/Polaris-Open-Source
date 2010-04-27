#include "StdAfx.h"
#include "syncer.h"
#include "FileEventLoop.h"
#include "SyncLeaderHandler.h"
#include "SyncFollowerHandler.h"
#include "../polaris/RecursiveRemoveDirectoryWithPrejudice.h"

Syncer::Syncer(FileEventLoop* files,
               std::wstring leader,
               std::wstring follower) : m_files(files), m_leader(leader)
{
	m_leaderSyncer = makeSyncLeaderHandler(leader, follower);
	m_leaderDir = leader.substr(0, leader.find_last_of('\\') + 1);
    files->watch(m_leaderDir, m_leaderSyncer);
	
	// Eventually, the leader and the follower syncers will use custom logic.
	m_followerSyncer = makeSyncFollowerHandler(leader, follower);
	m_followerDir = follower.substr(0, follower.find_last_of('\\') + 1);
    files->watch(m_followerDir, m_followerSyncer);
}

Syncer::~Syncer()
{
    m_files->ignore(m_leaderDir, m_leaderSyncer);
    m_files->ignore(m_followerDir, m_followerSyncer);
    if (!DeleteFile(m_leader.c_str())) {
        printf("DeleteFile() failed: %d\n", GetLastError());
    }
}