#pragma once

class ActivePet;
class FileEventLoop;

class ActiveWorld
{
    wchar_t* m_password;
	FileEventLoop* m_files;
    typedef std::map<std::wstring, ActivePet*> NAME_PET;
	NAME_PET m_pets;

public:
	ActiveWorld(wchar_t* password, FileEventLoop* files);
	virtual ~ActiveWorld();

	ActivePet* getActivePet(std::wstring petname);
    std::set<ActivePet*> getActivePets() const;
    void terminateActivePet(ActivePet* pet);
};
