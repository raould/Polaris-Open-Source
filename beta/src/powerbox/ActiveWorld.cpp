#include "StdAfx.h"
#include "ActiveWorld.h"
#include "ActivePet.h"
#include "EventLoop.h"

ActiveWorld::ActiveWorld(wchar_t* password, FileEventLoop* files) : m_password(password), m_files(files)
{
}

ActiveWorld::~ActiveWorld()
{
}

ActivePet* ActiveWorld::getActivePet(std::wstring petname){
	if (m_pets.find (petname) == m_pets.end()) {
		ActivePet* pet = new ActivePet(m_password, petname, m_files);
		m_pets[petname] = pet;
	}
	return m_pets[petname];
}

std::set<ActivePet*> ActiveWorld::getActivePets() const {
    std::set<ActivePet*> r;
    for (NAME_PET::const_iterator i = m_pets.begin(); i != m_pets.end(); ++i) {
        r.insert((*i).second);
    }
    return r;
}

void ActiveWorld::terminateActivePet(ActivePet* pet) {
    NAME_PET::iterator end = m_pets.upper_bound(pet->getName());
    NAME_PET::iterator i = m_pets.lower_bound(pet->getName());
    for (; i != end; ++i) {
        if (i->second == pet) {
            m_pets.erase(i);
            delete pet;
            break;
        }
    }
}
