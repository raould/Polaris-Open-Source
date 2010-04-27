// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct ActiveAccount
{
    /**
     * An operation on the active account.
     */
    struct Task {
        virtual ~Task() {}

        virtual void run(ActiveAccount& account) = 0;
    };

    /**
     * Get the petname for this account.
     */
    virtual std::wstring getPetname() = 0;

    /**
     * Determine the corresponding mirror directory.
     * @param dir   The path to the original directory.
     * @return The path to the pet's mirror directory.
     */
    virtual std::wstring mirror(std::wstring dir) = 0;

    /**
     * Determine the corresponding user directory.
     * @param path  The path to the pet file.
     * @return The path to the user's file.
     */
    virtual std::wstring invert(std::wstring path) = 0;

    /**
     * Create a synchronized local copy of a file.
     * @param file  The path to the original file.
	 * @param copy  Should the contents of the original file be copied.
     * @return The path to the pet's copy of the file.
     */
    virtual std::wstring edit(std::wstring file, bool copy) = 0;

	/**
     * Launch a specified executable.
     * @param exe   The path to the executable file.
	 * @param args  The command line arguments.
     * @return The created PID.
	 */
	virtual DWORD launch(std::wstring exe, std::wstring args) = 0;

    /**
     * Terminate any running applications in the pet and log off.
     */
    virtual void kill() = 0;

    struct Kill : public Task {
        virtual ~Kill() {}

        virtual void run(ActiveAccount& account) { account.kill(); }
    };
};

struct ActivePet
{
    virtual ~ActivePet() {}

    /**
     * Get this pet's petname.
     */
    virtual std::wstring getName() const = 0;

    /**
     * Get the SID identifying this pet.
     */
    virtual PSID getUserSID() const = 0;

    /**
     * Does this pet manage the given process?
     */
    virtual bool ownsProcess(HANDLE process) const = 0;

    /**
     * Provide powerbox service to a process.
     * @param process   The process to manage.
     * @return true if pet is now managing the process, else false.
     */
    virtual bool manage(HANDLE process) = 0;

    /**
     * Notification that a serviced process has terminated.
     * @param process   The managed process.
     * @return true if the pet should be terminated.
     */
    virtual bool release(HANDLE process) = 0;

    /**
     * Puts an account operation on the task queue.
     */
    virtual void send(ActiveAccount::Task* task) = 0;
};

ActivePet* openPet(std::wstring petname, wchar_t* user_password);
