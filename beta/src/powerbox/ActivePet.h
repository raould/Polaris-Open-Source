#pragma once

#include "../polaris/auto_handle.h"

class FileEventLoop;
class Syncer;
class MessageDispatcher;

class ActivePet
{
    static const int MAX_TICKS_WHILE_INVISIBLE = 3;

	std::wstring m_petname;
	FileEventLoop* m_files;

    auto_handle m_session;
    HANDLE m_profile;
    auto_handle m_job;
    std::wstring m_requestFolderPath;
    MessageDispatcher* m_dispatcher;
	unsigned int m_next_editable_name;
    unsigned int m_ticks_while_invisible;

	std::vector<Syncer*> m_syncers;
	
public:
	ActivePet(wchar_t* user_password, std::wstring petname, FileEventLoop* files);
	virtual ~ActivePet();

    /**
     * Gets the petname.
     */
    std::wstring getName() { return m_petname; }

    /**
     * Create a mirror directory in the pet account.
     * @param originalDirPath   The path to the original directory.
     * @return The path to the pet's mirror of the directory.
     */
    std::wstring mirror(std::wstring originalDirPath);

    /**
     * Create a synchronized local copy of a file.
     * @param originalDocPath   The path to the original file.
     * @return The path to the pet's copy of the file.
     */
    std::wstring edit(std::wstring originalDocPath);

	/**
	 * @param originalDocPath will be zero length string if there is no doc specified
	 * @return error code zero if things turned out ok
	 */
	int launch(std::wstring exePath, std::wstring originalDocPath);

    /**
     * Does this pet own the given process?
     */
    bool ownsProcess(HANDLE process);

    /**
     * Increase the age of this pet.
     * @return true if this pet should be terminated.
     */
    bool age();

    void resetAge();
};
