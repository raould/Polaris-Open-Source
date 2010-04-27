#include "StdAfx.h"
#include "Syncer.h"

#include "FileEventLoop.h"
#include "SyncLeaderHandler.h"
#include "SyncFollowerHandler.h"
#include "../polaris/RecursiveRemoveDirectoryWithPrejudice.h"
#include "../polaris/Runnable.h"

Syncer::Syncer(std::wstring leader,
               std::wstring follower) : m_leader(leader)
{
	m_leaderSyncer = makeSyncLeaderHandler(leader, follower);
	m_leaderDir = leader.substr(0, leader.find_last_of('\\') + 1);
    FileEventLoop::watch(m_leaderDir, m_leaderSyncer);
	
	// Eventually, the leader and the follower syncers will use custom logic.
	m_followerSyncer = makeSyncFollowerHandler(leader, follower);
	m_followerDir = follower.substr(0, follower.find_last_of('\\') + 1);
    FileEventLoop::watch(m_followerDir, m_followerSyncer);
}

Syncer::~Syncer()
{
    FileEventLoop::ignore(m_leaderDir, m_leaderSyncer);
    delete m_leaderSyncer;
    FileEventLoop::ignore(m_followerDir, m_followerSyncer);
    delete m_followerSyncer;
    if (!DeleteFile(m_leader.c_str())) {
        printf("DeleteFile() failed: %d\n", GetLastError());
    }
}