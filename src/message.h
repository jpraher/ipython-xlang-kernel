/* -----------------------------------------------------------------------------
 * Copyright (C) 2012 Jakob Praher
 *
 * Distributed under the terms of the BSD License. The full license is in
 * the file COPYING, distributed as part of this software.
 * -----------------------------------------------------------------------------
 */

#ifndef __message__h__
#define __message__h__

#include "json.h"
#include <zmq.hpp>

class Message {
public:
    virtual bool serialize(const std::string & key, std::list<zmq::message_t*> *messages) const = 0;
    virtual bool deserialize(const std::list<zmq::message_t*> & messages) = 0;

    static void free(std::list<zmq::message_t*> & messages);
};


class Channel {
public:
    virtual ~Channel() {}
    virtual void send(const Message& message) = 0;
};

class EContext {
public:
    EContext(Channel &io, Channel &shell);

    Channel & io();
    Channel & shell();
private:
    Channel * _io;
    Channel * _shell;
};

class ExecuteHandler {
public:
    virtual void execute(EContext & ctx,
                         std::list<zmq::message_t*> & request)  = 0;
};



#endif // __ipython__message__h__
