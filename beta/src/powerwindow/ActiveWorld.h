// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct ActivePet;
struct EventLoop;

struct ActiveWorld
{
    virtual ~ActiveWorld() {}

    virtual std::list<ActivePet*>& list() = 0;
	virtual ActivePet* findByName(std::wstring petname) = 0;
	virtual ActivePet* findByProcess(HANDLE process) = 0;
    virtual ActivePet* findByUser(PSID user_sid) = 0;
	virtual ActivePet* findByWindow(HWND window) = 0;
	virtual ActivePet* load(std::wstring petname) = 0;
};

ActiveWorld* makeActiveWorld(EventLoop* event_loop, const wchar_t* password);
