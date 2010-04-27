// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

struct ActiveAccount;
struct MessageHandler;

MessageHandler* makeGetOpenFileNameAHandler(ActiveAccount*);
