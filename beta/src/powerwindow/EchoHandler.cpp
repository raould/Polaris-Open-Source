// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "EchoHandler.h"

#include "MessageHandler.h"
#include "MessageResponder.h"
#include "../polaris/Message.h"

class EchoHandler : public MessageHandler
{
public:
    virtual void run(Message& request, MessageResponder* out) {
        int len = request.length();
        void* data;
        request.cast(&data, len);
        out->run(len, data);
    }
};
 
MessageHandler* makeEchoHandler() {
    return new EchoHandler();
}
