#pragma once

class FileEventLoop;
struct Runnable;

class Syncer
{
	FileEventLoop* m_files;
    std::wstring m_leader;
    std::wstring m_leaderDir;
    Runnable* m_leaderSyncer;
    std::wstring m_followerDir;
    Runnable* m_followerSyncer;

public:
	/**
	 * The syncer must handle a surprising arrays of conditions:
	 * The syncer may be initiated when neither the original nor the editable yet exists (during a SaveAs for example).
	 * If the app locks its files open, we must duplicate thelocking to the original from the editable
	 * if the original is updated by an outside app, the editable must be updated, 
	 * (i.e., the copying is bidirectional -- example, using notepad and firefox at the same time to cyclically edit and
	 * review html -- the firefox editable must be updated with the new version when the notepad syncer updates the original).
	 * The editable may be deleted temporarily and recreated by renaming of another file in the middle of operations (MS-Word).
	 * The editable may be modified in place (Notepad).
	 * The editable and original may be running with different clocks (modify times unsynced).
	 * The syncer must be able to learn that it is time to die, i.e., it must watch for
	 * a terminate sync message.
	 *
	 * @param leader path to file that is typically the editable, i.e., the one most likely to be changed, and the one
	 *        which we look at to see if it is locked -- if it is locked, we lock the follower as well
	 * @param follower path to file that is typically the original, i.e., the one to be updated.
	 */
	Syncer(FileEventLoop* files, std::wstring leader, std::wstring follower);
	virtual ~Syncer();
};
