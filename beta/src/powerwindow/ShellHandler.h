// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

#include "ActivePet.h"

struct ActiveWorld;
struct MessageHandler;
struct MessageResponder;

MessageHandler* makeShellHandler(ActiveWorld*);

ActiveAccount::Task* makeShellTask(std::wstring, MessageResponder*);
